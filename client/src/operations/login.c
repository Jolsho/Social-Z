/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "networking/context.h"
#include "operations/login.h"
#include "networking/dispatch.h"
#include "codec/blob.h"
#include <sodium.h>
#include <stdlib.h>

void login_cleanup(struct Client* cli, ContextID id) {
    if (cli->login_id == id) {
        LoginOperation* login = cli->net.states[id].operation;
        blob_discard_partial(cli, id, &login->blob);

        Buffer* password = &login->password;
        if (password->b) {
            sodium_memzero(password->b, password->cap);
            buffer_pool_push(&cli->pool, password->b, password->cap);
        }
        sodium_memzero(login, sizeof(*login));
        free(login);
        cli->login_id = 0;
    }
}

static int login_send_request(struct Client* cli, ContextID id) {
    ConState* state = &cli->net.states[id];
    LoginOperation* login = state->operation;
    if (state->state != CON_IDLE || state->send_owned) {
        return CLIENT_CONN_BUSY;
    }

    Buffer* request = &cli->net.send_buffers[id];
    uint32_t size = 6 + account_username_size(login->username);

    if (buffer_ensure_min_cap(cli, request, size) != CLIENT_OK) {
        client_free_context(cli, id);
        return CLIENT_ERR;
    }

    marshal_account_request(request->b, login->username);
    request->size = size;
    state->send_owned = true;
    state->state = CON_RECEIVING;

    // The host owns this buffer until it returns it, independently of replies.
    send_request(cli, id, request);

    if (cli->logged_in) {
        return CLIENT_PARSE_DONE;
    }

    return cli->login_id == id ? CLIENT_OK : CLIENT_ERR;
}

int client_login(
    struct Client* cli,
    const char* username,
    const uint8_t* password,
    size_t password_size,
    ContextID* id
) {
    if (!cli || !id || !password || !password_size ||
        password_size > LOGIN_PASSWORD_MAX || !account_username_size(username)) {
        return CLIENT_ERR;
    }
    if (cli->logged_in || cli->login_id) {
        return CLIENT_CONN_BUSY;
    }

    ContextID context = client_new_context(&cli->net);
    if (!valid_id(context)) {
        return CLIENT_CONN_BUSY;
    }

    LoginOperation* login = calloc(1, sizeof(*login));
    if (!login) {
        client_free_context(cli, context);
        return CLIENT_ERR;
    }
    cli->net.states[context].parser_id = PARSER_ID_USER_DATA;
    cli->net.states[context].operation = login;
    cli->login_id = context;

    size_t capacity = password_size;
    uint8_t* secret = buffer_pool_pop(&cli->pool, &capacity);
    if (!secret) {
        client_free_context(cli, context);
        return CLIENT_ERR;
    }

    login->password = (Buffer){secret, capacity, password_size};
    memcpy(secret, password, password_size);
    memcpy(login->username, username, account_username_size(username) + 1);

    *id = context;

    return login_send_request(cli, context);
}

int client_cancel_login(struct Client* cli) {
    if (!cli) {
        return CLIENT_ERR;
    }
    if (cli->login_id) {
        client_free_context(cli, cli->login_id);
    }

    return CLIENT_OK;
}

bool client_is_logged_in(const struct Client* cli) {
    return cli && cli->logged_in;
}

int client_get_public_key(const struct Client* cli, uint8_t public_key[32]) {
    if (!cli || !public_key || !cli->logged_in) {
        return CLIENT_ERR;
    }

    memcpy(public_key, cli->keys.pub.b, KEY_SIZE);

    return CLIENT_OK;
}

int login_handle_response(
    struct Client* cli,
    ContextID id,
    uint8_t* b,
    uint64_t len
) {
    if (!valid_id(id)) {
        return CLIENT_INVALID_ID;
    }
    if (!cli || cli->login_id != id) {
        return CLIENT_ERR;
    }

    LoginOperation* login = cli->net.states[id].operation;
    int r = CLIENT_ERR;

    if (!b) {
        goto done;
    }

    if (!login->metadata_ready) {
        if (len != ACCOUNT_RESPONSE_METADATA_SIZE ||
            parse_account_response_metadata(&login->metadata, b, (size_t)len) != 0 ||
            login->metadata.size != LOGIN_BLOB_SIZE) {
            goto done;
        }
        login->metadata_ready = true;
        // The node sends the encrypted header next; no second request is needed.
        return CLIENT_OK;
    }
    if (len < HASH_SIZE || len > HASH_SIZE + sizeof(uint64_t) + LOGIN_BLOB_SIZE ||
        memcmp(b, login->metadata.hash.b, HASH_SIZE) != 0) {
        goto done;
    }
    if (!login->blob.active) {
        uint64_t size;
        if (len < HASH_SIZE + sizeof(size)) {
            goto done;
        }
        memcpy(&size, b + HASH_SIZE, sizeof(size));
        if (size != login->metadata.size) {
            goto done;
        }
    }

    r = parse_blob(cli, id, &login->blob, b, len);
    if (r == CLIENT_OK) {
        return r;
    }
    if (r != CLIENT_PARSE_DONE) {
        goto done;
    }

    StoreItem* item = store_get_item(&cli->blob_store, &login->metadata.hash);
    r = CLIENT_ERR;

    if (!item || item->size != LOGIN_BLOB_SIZE || item->received != item->size) {
        goto done;
    }
    if (decrypt_account_header(
        &cli->account, &cli->keys, &login->metadata.account,
        login->username, login->password.b, login->password.size, item->b, item->size
    ) == 0) {
        cli->logged_in = true;
        r = CLIENT_PARSE_DONE;
    }

done:
    client_free_context(cli, id);

    return r;
}
