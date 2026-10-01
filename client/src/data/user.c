/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_client/client.h"
#include "netwrk/context.h"
#include "netwrk/parsers.h"
#include <sodium.h>

#define USER_DATA_REQUEST_SIZE      KEY_SIZE

int marshal_get_user_data_request(
    struct Client* cli, ContextID id
) {
    if (!valid_id(id)) return CLIENT_INVALID_ID;

    ConState* state = &cli->net.states[id];
    if (state->state != CON_IDLE) return CLIENT_CONN_BUSY;


    size_t cap = USER_DATA_REQUEST_SIZE;
    uint8_t* bytes = buffer_pool_pop(&cli->pool, &cap);

    Buffer buff = {
        .b = bytes,
        .cap = cap,
        .size = 0,
    };



    memcpy(buff.b, cli->keys.pub.b, KEY_SIZE);
    buff.b += KEY_SIZE;
    buff.size += KEY_SIZE;

    state->parser_id = PARSER_ID_USER_DATA;
    state->state = CON_SENDING;

    // TODO => append to send buffers.

    return CLIENT_OK;
}

int parse_user_data(
    struct Client* cli, ContextID id, 
    uint8_t* b, uint64_t len
) {

    size_t remaining = len - (NONCE_SIZE + sizeof(size_t) + crypto_pwhash_saltbytes());
    if (remaining <= 0) return CLIENT_SMALL_BUFFER;

    if (crypto_sign_ed25519_sk_to_pk(cli->keys.pub.b, cli->keys.priv.b) != 0) {
        return CLIENT_ERR;
    }

    uint8_t salt[crypto_pwhash_saltbytes()];
    memcpy(salt, b, crypto_pwhash_saltbytes());
    b += crypto_pwhash_saltbytes();

    Nonce nonce;
    memcpy(nonce.b, b, NONCE_SIZE);
    b += NONCE_SIZE;

    size_t pswd_len = 0;
    memcpy(&pswd_len, b, sizeof(size_t));
    b += sizeof(size_t);

    if ((remaining -= pswd_len) <= 0) {
        return CLIENT_ERR;
    }

    if (crypto_pwhash(
        cli->data_key.b, KEY_SIZE,
        (char*)b, remaining,
        salt,
        crypto_pwhash_OPSLIMIT_MODERATE,
        crypto_pwhash_MEMLIMIT_MODERATE,
        crypto_pwhash_ALG_DEFAULT
    ) != 0) {
        return CLIENT_ERR;
    }

    unsigned long long cli_data_len = remaining;
    if (crypto_aead_chacha20poly1305_decrypt(
        b, &cli_data_len, 
        NULL,
        b, remaining,
        NULL, 0,
        nonce.b, 
        cli->data_key.b
    ) != 0) {
        return CLIENT_ERR;
    }

    // TODO -- start parsing user data.

    return CLIENT_PARSE_DONE;
}
