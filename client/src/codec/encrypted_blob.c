/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "codec/encrypted_blob.h"
#include <sodium.h>

static void write_uint(uint8_t* out, uint64_t value, size_t size) {
    while (size) {
        out[--size] = (uint8_t)value;
        value >>= 8;
    }
}

int encrypt_blob_header(BlobCrypt* ctx, uint8_t* header, const Key* key) {
    if (!ctx || !header || !key) {
        return -1;
    }

    memset(ctx, 0, sizeof(*ctx));
    // Version 1 and suite 1 select our secretstream envelope.
    ctx->header[1] = 1;
    ctx->header[3] = 1;
    if (crypt_init_encrypt(&ctx->crypt, ctx->header + 4, key) != 0) {
        return -1;
    }

    memcpy(header, ctx->header, sizeof(ctx->header));
    return 0;
}

int decrypt_blob_header(
    BlobCrypt* ctx,
    const uint8_t* header,
    size_t size,
    const Key* key
) {
    if (!ctx || !header || !key || size != ENCRYPTED_BLOB_HEADER_SIZE ||
        header[0] != 0 || header[1] != 1 || header[2] != 0 || header[3] != 1) {
        return -1;
    }

    memset(ctx, 0, sizeof(*ctx));
    memcpy(ctx->header, header, sizeof(ctx->header));
    return crypt_init_decrypt(&ctx->crypt, ctx->header + 4, key);
}

int encrypt_blob_chunk(BlobCrypt* ctx, Buffer* buffer, bool final) {
    if (!ctx || ctx->crypt.mode != CRYPT_ENCRYPTING || !buffer || !buffer->b ||
        buffer->size > buffer->cap || buffer->size > CRYPT_RECORD_MAX ||
        buffer->cap - buffer->size < ENCRYPTED_CHUNK_OVERHEAD ||
        ctx->chunk_number == UINT64_MAX) {
        return -1;
    }

    uint8_t ad[ENCRYPTED_BLOB_HEADER_SIZE + 8];
    memcpy(ad, ctx->header, sizeof(ctx->header));
    write_uint(ad + sizeof(ctx->header), ctx->chunk_number, 8);

    // Reserve framing bytes in the same allocation handed to the networking host.
    memmove(buffer->b + ENCRYPTED_CHUNK_PREFIX_SIZE, buffer->b, buffer->size);
    Buffer chunk = {
        buffer->b + ENCRYPTED_CHUNK_PREFIX_SIZE,
        buffer->cap - ENCRYPTED_CHUNK_PREFIX_SIZE,
        buffer->size,
    };
    if (encrypt(&ctx->crypt, &chunk, ad, sizeof(ad), final) != 0) {
        sodium_memzero(buffer->b, buffer->size + ENCRYPTED_CHUNK_PREFIX_SIZE + 1);
        buffer->size = 0;
        return -1;
    }

    write_uint(buffer->b, chunk.size, ENCRYPTED_CHUNK_PREFIX_SIZE);
    buffer->size = ENCRYPTED_CHUNK_PREFIX_SIZE + chunk.size;
    ctx->chunk_number++;
    return 0;
}

int decrypt_blob_chunk(BlobCrypt* ctx, Buffer* buffer, bool final) {
    if (!ctx || ctx->crypt.mode != CRYPT_DECRYPTING || !buffer || !buffer->b ||
        buffer->size > buffer->cap || buffer->size < ENCRYPTED_CHUNK_OVERHEAD ||
        buffer->size > CRYPT_RECORD_MAX + ENCRYPTED_CHUNK_OVERHEAD ||
        ctx->chunk_number == UINT64_MAX) {
        return -1;
    }

    uint32_t size = 0;
    for (size_t i = 0; i < ENCRYPTED_CHUNK_PREFIX_SIZE; i++) {
        size = (size << 8) | buffer->b[i];
    }
    if (size != buffer->size - ENCRYPTED_CHUNK_PREFIX_SIZE) {
        return -1;
    }

    uint8_t ad[ENCRYPTED_BLOB_HEADER_SIZE + 8];
    memcpy(ad, ctx->header, sizeof(ctx->header));
    write_uint(ad + sizeof(ctx->header), ctx->chunk_number, 8);
    Buffer chunk = {
        buffer->b + ENCRYPTED_CHUNK_PREFIX_SIZE,
        buffer->cap - ENCRYPTED_CHUNK_PREFIX_SIZE,
        size,
    };
    if (decrypt(&ctx->crypt, &chunk, ad, sizeof(ad), final) != 0) {
        sodium_memzero(buffer->b, buffer->size);
        buffer->size = 0;
        return -1;
    }

    memmove(buffer->b, chunk.b, chunk.size);
    memset(buffer->b + chunk.size, 0, ENCRYPTED_CHUNK_OVERHEAD);
    buffer->size = chunk.size;
    ctx->chunk_number++;
    return 0;
}
