/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz/api/vec.h"
#include "sz/codec.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// USER

typedef struct {
    KeyPair     keys;
    Key         user_data_key;
} UserCtx;

Vec* marshal_user_data_request(uint8_t* pub_key);
UserCtx* create_user_ctx_from_login_response(uint8_t* buff, size_t len); 



// CRYPTO

#define TAG_LEN 16
typedef struct CryptCtx {
    Key             remote;
    Key             sym;
    Nonce           nonce;
    unsigned char   tag[TAG_LEN];
    unsigned char*  AD;
    size_t          AD_LEN;
} CryptCtx;

CryptCtx* new_crypto_ctx(UserCtx* user, uint8_t* head, size_t head_len);
bool decrypt(CryptCtx* ctx, uint8_t* buf, size_t len);
bool encrypt(CryptCtx* ctx, uint8_t* buf, size_t len);


#ifdef __cplusplus
}
#endif
