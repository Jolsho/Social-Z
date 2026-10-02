/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "sz_common/codec.h"
#include "utils/buffers.h"
#include <stdbool.h>
#include <sodium/crypto_secretstream_xchacha20poly1305.h>

#define CRYPT_HEADER_SIZE 24
#define CRYPT_RECORD_OVERHEAD 17
#define CRYPT_RECORD_MAX (64 * 1024)

typedef struct CryptCtx {
    crypto_secretstream_xchacha20poly1305_state stream;
    enum { CRYPT_CLOSED, CRYPT_ENCRYPTING, CRYPT_DECRYPTING } mode;
} CryptCtx;

int crypt_init_encrypt(CryptCtx* ctx, uint8_t header[CRYPT_HEADER_SIZE], const Key* key);
int crypt_init_decrypt(CryptCtx* ctx, const uint8_t header[CRYPT_HEADER_SIZE], const Key* key);
void crypt_clear(CryptCtx* ctx);

/* Process one record in caller-owned storage, with no allocation.
 * Encryption needs size + CRYPT_RECORD_OVERHEAD bytes of capacity.
 * Additional data is borrowed and must not overlap the buffer or context.
 * Callers frame records and pass final=true only for the last record.
 * Successful final records wipe and close the context.
 * Invalid arguments leave the buffer unchanged.
 * Authentication or unexpected-tag failure wipes the buffer, sets size=0, and closes the context.
 * Return 0 or -1; never expose failed decryption as usable plaintext.
 */
int encrypt(CryptCtx* ctx, Buffer* buffer, const uint8_t* ad, size_t ad_size, bool final);
int decrypt(CryptCtx* ctx, Buffer* buffer, const uint8_t* ad, size_t ad_size, bool final);
