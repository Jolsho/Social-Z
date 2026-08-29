/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "utils/crypto.h"
#include "sz/crypto.h"
#include <sodium/crypto_pwhash.h>
#include <string.h>
#include <sodium.h>

const size_t MAX_AD_LEN = 32;

bool decrypt(CryptCtx* ctx, uint8_t* buf, size_t len) {
    int s = encoded_key_len();
    int r = crypto_aead_chacha20poly1305_decrypt_detached(
        buf, NULL,
        buf, len,
        ctx->tag,
        ctx->AD, AD_LEN, 
        ctx->nonce.b, ctx->sym.b
    );
    return r == 0;
}

bool encrypt(CryptCtx* ctx, uint8_t* buf, size_t len) {
    unsigned long long tag_len = TAG_LEN;
    int r = crypto_aead_chacha20poly1305_encrypt_detached(
        buf, ctx->tag, &tag_len,
        buf, len, 
        ctx->AD, AD_LEN, 
        NULL, ctx->nonce.b, ctx->sym.b
    );
    return r == 0;
}

