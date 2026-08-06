/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz/client/client.h"
#include "sz/crypto.h"
#include <sodium/crypto_pwhash.h>
#include <string.h>
#include <sodium.h>
const size_t MAX_AD_LEN = 32;

CryptCtx* new_crypto_ctx(UserCtx* user, uint8_t* head, size_t head_len) {
    CryptCtx* ctx = malloc(sizeof(CryptCtx));
    uint8_t* b = head;
    memcpy(ctx->remote.b, b, KEY_SIZE); 
    b += KEY_SIZE;

    memcpy(&ctx->AD_LEN, b, sizeof(uint8_t)); 
    b += sizeof(uint8_t);

    if (ctx->AD_LEN > MAX_AD_LEN) { return NULL; }

    ctx->AD = malloc(ctx->AD_LEN);
    memcpy(ctx->AD, b, ctx->AD_LEN); b += ctx->AD_LEN;

    return ctx;
}

bool decrypt(CryptCtx* ctx, uint8_t* buf, size_t len) {
    int s = encoded_key_len();
    int r = crypto_aead_chacha20poly1305_decrypt_detached(
        buf, NULL,
        buf, len,
        ctx->tag,
        ctx->AD, ctx->AD_LEN, 
        ctx->nonce.b, ctx->sym.b
    );
    return r == 0;
}

bool encrypt(CryptCtx* ctx, uint8_t* buf, size_t len) {
    unsigned long long tag_len = TAG_LEN;
    int r = crypto_aead_chacha20poly1305_encrypt_detached(
        buf, ctx->tag, &tag_len,
        buf, len, 
        ctx->AD, ctx->AD_LEN, 
        NULL, ctx->nonce.b, ctx->sym.b
    );
    return r == 0;
}

