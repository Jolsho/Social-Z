/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "utils/crypto.h"
#include <sodium.h>

_Static_assert(CRYPT_HEADER_SIZE == crypto_secretstream_xchacha20poly1305_HEADERBYTES, "Stream header size");
_Static_assert(CRYPT_RECORD_OVERHEAD == crypto_secretstream_xchacha20poly1305_ABYTES, "Record overhead");
_Static_assert(KEY_SIZE == crypto_secretstream_xchacha20poly1305_KEYBYTES, "Stream key size");

void crypt_clear(CryptCtx* ctx) {
    if (ctx) {
        sodium_memzero(ctx, sizeof(*ctx));
    }
}

int crypt_init_encrypt(CryptCtx* ctx, uint8_t header[CRYPT_HEADER_SIZE], const Key* key) {
    if (!ctx || !header || !key || sodium_init() < 0) {
        return -1;
    }

    crypt_clear(ctx);
    if (crypto_secretstream_xchacha20poly1305_init_push(&ctx->stream, header, key->b) != 0) {
        crypt_clear(ctx);
        return -1;
    }

    ctx->mode = CRYPT_ENCRYPTING;
    return 0;
}

int crypt_init_decrypt(CryptCtx* ctx, const uint8_t header[CRYPT_HEADER_SIZE], const Key* key) {
    if (!ctx || !header || !key || sodium_init() < 0) {
        return -1;
    }

    crypt_clear(ctx);
    if (crypto_secretstream_xchacha20poly1305_init_pull(&ctx->stream, header, key->b) != 0) {
        crypt_clear(ctx);
        return -1;
    }

    ctx->mode = CRYPT_DECRYPTING;
    return 0;
}

static int failed_record(CryptCtx* ctx, Buffer* buffer) {
    sodium_memzero(buffer->b, buffer->size);
    buffer->size = 0;
    crypt_clear(ctx);
    return -1;
}

int encrypt(CryptCtx* ctx, Buffer* buffer, const uint8_t* ad, size_t ad_size, bool final) {
    if (!ctx || ctx->mode != CRYPT_ENCRYPTING || !buffer || !buffer->b ||
        buffer->size > buffer->cap || buffer->size > CRYPT_RECORD_MAX ||
        buffer->cap - buffer->size < CRYPT_RECORD_OVERHEAD || (ad_size && !ad)) {
        return -1;
    }

    uint8_t tag = final ? crypto_secretstream_xchacha20poly1305_TAG_FINAL
                        : crypto_secretstream_xchacha20poly1305_TAG_MESSAGE;

    // Secretstream prepends one tag byte; align input with the ciphertext payload for exact overlap.
    memmove(buffer->b + 1, buffer->b, buffer->size);
    if (crypto_secretstream_xchacha20poly1305_push(
        &ctx->stream, buffer->b, NULL, buffer->b + 1, buffer->size, ad, ad_size, tag
    ) != 0) {
        // The shifted plaintext occupies one extra byte on this failure path.
        buffer->size++;
        return failed_record(ctx, buffer);
    }

    buffer->size += CRYPT_RECORD_OVERHEAD;
    if (final) {
        crypt_clear(ctx);
    }
    return 0;
}

int decrypt(CryptCtx* ctx, Buffer* buffer, const uint8_t* ad, size_t ad_size, bool final) {
    if (!ctx || ctx->mode != CRYPT_DECRYPTING || !buffer || !buffer->b ||
        buffer->size > buffer->cap || buffer->size < CRYPT_RECORD_OVERHEAD ||
        buffer->size > CRYPT_RECORD_MAX + CRYPT_RECORD_OVERHEAD || (ad_size && !ad)) {
        return -1;
    }

    uint8_t tag;
    uint8_t expected = final ? crypto_secretstream_xchacha20poly1305_TAG_FINAL
                             : crypto_secretstream_xchacha20poly1305_TAG_MESSAGE;

    // Authentication completes before libsodium overwrites the ciphertext payload.
    if (crypto_secretstream_xchacha20poly1305_pull(
        &ctx->stream, buffer->b + 1, NULL, &tag, buffer->b, buffer->size, ad, ad_size
    ) != 0 || tag != expected) {
        return failed_record(ctx, buffer);
    }

    uint32_t plain_size = buffer->size - CRYPT_RECORD_OVERHEAD;
    memmove(buffer->b, buffer->b + 1, plain_size);
    sodium_memzero(buffer->b + plain_size, CRYPT_RECORD_OVERHEAD);
    buffer->size = plain_size;

    if (final) {
        crypt_clear(ctx);
    }
    return 0;
}
