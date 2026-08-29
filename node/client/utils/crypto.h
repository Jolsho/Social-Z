
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

////////////////////////// CRYPTO ////////////////////////
#define TAG_LEN 16
#define AD_LEN 8
typedef struct CryptCtx {
    Key             remote;
    Key             sym;
    Nonce           nonce;
    unsigned char   tag[TAG_LEN];
    unsigned char   AD [AD_LEN];
} CryptCtx;

bool decrypt(CryptCtx* ctx, uint8_t* buf, size_t len);
bool encrypt(CryptCtx* ctx, uint8_t* buf, size_t len);

