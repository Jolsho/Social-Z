/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "netwrk/context.h"
#include "netwrk/marshalers.h"

int send_user_data_request(struct Client* cli, ContextID id) {
    int r = marshal_get_user_data_request(cli, id);
    if (r != CLIENT_OK) {
        client_free_context(cli, id);
        return r;
    }

    cli->net.states[id].send_owned = true;
    cli->net.states[id].state = CON_RECEIVING;

    send_request(cli, id, &cli->net.send_buffers[id]);

    if (cli->logged_in)
        return CLIENT_PARSE_DONE;

    return cli->login.id == id ? CLIENT_OK : CLIENT_ERR;
}

void client_return_buffer(struct Client* cli, struct Buffer* buff) {
    if (!cli || !buff || !cli->net.send_buffers)
        return;

    for (ContextID id = 1; id < MAX_CONNS; id++) {
        ConState* state = &cli->net.states[id];
        if (buff != &cli->net.send_buffers[id] || !state->send_owned)
            continue;

        state->send_owned = false;
        context_release_send_buffer(cli, id);

        if (state->release_pending)
            client_free_context(cli, id);
        else if (cli->login.id == id && cli->login.lookup_ready && !cli->login.fetching) {
            state->state = CON_IDLE;
            send_user_data_request(cli, id);
        }
        return;
    }
}

int client_login(
    struct Client* cli,
    const char* username,
    const uint8_t* password,
    size_t password_size,
    ContextID* id
) {
    if (!cli || !id || !password || !password_size ||
        password_size > LOGIN_PASSWORD_MAX || !login_username_size(username))
        return CLIENT_ERR;
    if (cli->logged_in || cli->login.id)
        return CLIENT_CONN_BUSY;

    ContextID context = client_new_context(&cli->net);
    if (!valid_id(context))
        return CLIENT_CONN_BUSY;

    size_t capacity = password_size;
    uint8_t* secret = buffer_pool_pop(&cli->pool, &capacity);
    if (!secret) {
        client_free_context(cli, context);
        return CLIENT_ERR;
    }

    cli->login.id = context;
    cli->login.password = (Buffer){secret, capacity, password_size};
    memcpy(secret, password, password_size);
    memcpy(cli->login.username, username, login_username_size(username) + 1);

    *id = context;

    return send_user_data_request(cli, context);
}

int client_cancel_login(struct Client* cli) {
    if (!cli)
        return CLIENT_ERR;
    if (cli->login.id)
        client_free_context(cli, cli->login.id);

    return CLIENT_OK;
}

bool client_is_logged_in(const struct Client* cli) {
    return cli && cli->logged_in;
}

int client_get_public_key(const struct Client* cli, uint8_t public_key[32]) {
    if (!cli || !public_key || !cli->logged_in)
        return CLIENT_ERR;

    memcpy(public_key, cli->keys.pub.b, KEY_SIZE);

    return CLIENT_OK;
}
