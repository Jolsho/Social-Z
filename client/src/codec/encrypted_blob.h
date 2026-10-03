/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "utils/crypto.h"

#define ENCRYPTED_BLOB_HEADER_SIZE (4 + CRYPT_HEADER_SIZE)
#define ENCRYPTED_CHUNK_PREFIX_SIZE 4
#define ENCRYPTED_CHUNK_OVERHEAD (ENCRYPTED_CHUNK_PREFIX_SIZE + CRYPT_RECORD_OVERHEAD)

typedef struct BlobCrypt {
    CryptCtx crypt;
    uint8_t header[ENCRYPTED_BLOB_HEADER_SIZE];
    uint64_t chunk_number;
} BlobCrypt;

/* Envelope: version:u16=1, suite:u16=1 (secretstream), then stream header:24.
 * Chunks: ciphertext length:u32, then ciphertext, including its authentication tag.
 * All integers are big-endian. A blob must contain at least one chunk, ending in FINAL.
 * Header and chunk buffers are caller-owned and must not overlap the context.
 * Clear abandoned contexts with crypt_clear(&ctx->crypt).
 */
int encrypt_blob_header(BlobCrypt* ctx, uint8_t* header, const Key* key);
int decrypt_blob_header(BlobCrypt* ctx, const uint8_t* header, size_t size, const Key* key);

/* Encryption starts with plaintext at buffer->b and reserves ENCRYPTED_CHUNK_OVERHEAD.
 * Decryption starts with one complete length-prefixed chunk and returns plaintext in place.
 * Neither call allocates. final must match the last chunk in the declared whole-blob size.
 * Header and chunk ordinal are authenticated. Success advances the ordinal or closes FINAL.
 * Return 0 or -1. Invalid arguments leave bytes unchanged; authentication failure wipes them.
 * Structural failures leave the context unchanged; the operation must stop and clear it.
 * Transport fragments must be assembled into an owned chunk before decryption.
 */
int encrypt_blob_chunk(BlobCrypt* ctx, Buffer* buffer, bool final);
int decrypt_blob_chunk(BlobCrypt* ctx, Buffer* buffer, bool final);
