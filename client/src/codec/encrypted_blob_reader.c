/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "codec/encrypted_blob_reader.h"
#include <sodium.h>

int parse_encrypted_blob(
    EncryptedBlobReader* reader,
    const Key* key,
    Buffer* buffer,
    const uint8_t* bytes,
    size_t size,
    size_t* consumed
) {
    if (!reader || !key || !buffer || !buffer->b || !consumed || (size && !bytes)) {
        return ENCRYPTED_BLOB_ERR;
    }

    *consumed = 0;
    buffer->size = 0;
    if (reader->state == ENCRYPTED_READ_FAILED || size > reader->remaining ||
        (reader->state == ENCRYPTED_READ_HEADER &&
         reader->remaining + reader->field_received
             < ENCRYPTED_BLOB_HEADER_SIZE + ENCRYPTED_CHUNK_OVERHEAD)) {
        goto failed;
    }
    if (reader->state == ENCRYPTED_READ_END) {
        return ENCRYPTED_BLOB_DONE;
    }

    while (*consumed < size) {
        if (reader->state == ENCRYPTED_READ_HEADER || reader->state == ENCRYPTED_READ_LENGTH) {
            size_t field_size = reader->state == ENCRYPTED_READ_HEADER
                ? ENCRYPTED_BLOB_HEADER_SIZE : ENCRYPTED_CHUNK_PREFIX_SIZE;
            size_t take = field_size - reader->field_received;
            if (take > size - *consumed) {
                take = size - *consumed;
            }
            memcpy(reader->field + reader->field_received, bytes + *consumed, take);
            reader->field_received += take;
            reader->remaining -= take;
            *consumed += take;
            if (reader->field_received != field_size) {
                continue;
            }
            reader->field_received = 0;

            if (reader->state == ENCRYPTED_READ_HEADER) {
                if (decrypt_blob_header(&reader->crypt, reader->field, field_size, key) != 0) {
                    goto failed;
                }
                reader->state = ENCRYPTED_READ_LENGTH;
                continue;
            }

            uint32_t length = 0;
            for (size_t i = 0; i < ENCRYPTED_CHUNK_PREFIX_SIZE; i++) {
                length = (length << 8) | reader->field[i];
            }
            if (length < CRYPT_RECORD_OVERHEAD ||
                length > CRYPT_RECORD_MAX + CRYPT_RECORD_OVERHEAD ||
                length > reader->remaining ||
                buffer->cap < length + ENCRYPTED_CHUNK_PREFIX_SIZE) {
                goto failed;
            }

            // Only copy ciphertext into scratch after validating its declared length.
            memcpy(buffer->b, reader->field, ENCRYPTED_CHUNK_PREFIX_SIZE);
            reader->chunk_size = length + ENCRYPTED_CHUNK_PREFIX_SIZE;
            reader->chunk_received = ENCRYPTED_CHUNK_PREFIX_SIZE;
            reader->state = ENCRYPTED_READ_CHUNK;
        }

        size_t take = reader->chunk_size - reader->chunk_received;
        if (take > size - *consumed) {
            take = size - *consumed;
        }
        memcpy(buffer->b + reader->chunk_received, bytes + *consumed, take);
        reader->chunk_received += take;
        reader->remaining -= take;
        *consumed += take;
        if (reader->chunk_received != reader->chunk_size) {
            continue;
        }

        // The declared whole-blob size determines which chunk must carry FINAL.
        bool final = reader->remaining == 0;
        buffer->size = reader->chunk_size;
        if (decrypt_blob_chunk(&reader->crypt, buffer, final) != 0 ||
            (!final && reader->remaining < ENCRYPTED_CHUNK_OVERHEAD)) {
            goto failed;
        }
        reader->state = final ? ENCRYPTED_READ_END : ENCRYPTED_READ_LENGTH;
        return final ? ENCRYPTED_BLOB_DONE : ENCRYPTED_BLOB_CHUNK;
    }

    if (!reader->remaining) {
        goto failed;
    }
    return ENCRYPTED_BLOB_MORE;

failed:
    sodium_memzero(buffer->b, buffer->cap);
    buffer->size = 0;
    crypt_clear(&reader->crypt.crypt);
    reader->state = ENCRYPTED_READ_FAILED;
    return ENCRYPTED_BLOB_ERR;
}

int finish_encrypted_blob(EncryptedBlobReader* reader) {
    if (reader->state == ENCRYPTED_READ_END) {
        return ENCRYPTED_BLOB_DONE;
    }

    crypt_clear(&reader->crypt.crypt);
    reader->state = ENCRYPTED_READ_FAILED;
    return ENCRYPTED_BLOB_ERR;
}
