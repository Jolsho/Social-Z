/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz/client/client.h"
#include "sz/api/vec.h"
#include "sz/utils/vec.h"
#include <sodium/crypto_aead_chacha20poly1305.h>
#include <sodium/crypto_pwhash.h>
#include <string.h>
#include <stdlib.h>
#include <sodium.h>

#define USER_DATA_REQUEST_SIZE 64
Vec* marshal_user_data_request(uint8_t* pub_key) {
    Vec* v = new_vec(USER_DATA_REQUEST_SIZE);
    vec_write(v, pub_key, KEY_SIZE);
    return v;
}


UserCtx* create_user_ctx_from_user_data_response(
    uint8_t* priv_key, 
    uint8_t* resp, size_t len
) {
    size_t remaining = len - (NONCE_SIZE + sizeof(size_t) + crypto_pwhash_saltbytes());
    if (remaining <= 0) { return NULL; }

    uint8_t* c = resp;
    UserCtx* ctx = (UserCtx*)malloc(sizeof(UserCtx));
    memcpy(ctx->keys.priv.b, priv_key, KEY_SIZE);

    memcpy(
        ctx->keys.pub.b,
        ctx->keys.priv.b + crypto_sign_SEEDBYTES,
        crypto_sign_PUBLICKEYBYTES
    );

    if (crypto_scalarmult_base(ctx->keys.pub.b, ctx->keys.priv.b) != 0) {
        free(ctx);
        return NULL;
    }

    uint8_t salt[crypto_pwhash_saltbytes()];
    memcpy(salt, c, crypto_pwhash_saltbytes());
    c += crypto_pwhash_saltbytes();

    Nonce nonce;
    memcpy(nonce.b, c, NONCE_SIZE);
    c += NONCE_SIZE;

    size_t pswd_len = 0;
    memcpy(&pswd_len, c, sizeof(size_t));
    c += sizeof(size_t);

    if ((remaining -= pswd_len) <= 0) {
        free(ctx);
        return NULL;
    }

    if (crypto_pwhash(
        ctx->user_data_key.b, KEY_SIZE,
        (char*)c, remaining,
        salt,
        crypto_pwhash_OPSLIMIT_MODERATE,
        crypto_pwhash_MEMLIMIT_MODERATE,
        crypto_pwhash_ALG_DEFAULT
    ) != 0) {
        free(ctx);
        return NULL;
    }

    unsigned long long usr_data_len = remaining;
    if (crypto_aead_chacha20poly1305_decrypt(
        c, &usr_data_len, 
        NULL,
        c, remaining,
        NULL, 0,
        nonce.b, 
        ctx->user_data_key.b
    ) != 0) {
        free(ctx);
        return NULL;
    }

    // TODO -- start parsing user data.

    return ctx;
}


