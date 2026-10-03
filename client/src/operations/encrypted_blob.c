/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "operations/encrypted_blob.h"
#include "networking/context.h"
#include <sodium.h>

void encrypted_blob_clear(struct Client* cli, ContextID id, EncryptedBlobLoad* load) {
    blob_discard_partial(cli, id, &load->transfer);
    sodium_memzero(load, sizeof(*load));

    Buffer* scratch = &cli->net.recv_buffers[id];
    if (scratch->b) {
        // Scratch held plaintext and will be returned to the shared networking pool.
        sodium_memzero(scratch->b, scratch->cap);
    }
}

int encrypted_blob_read(
    struct Client* cli,
    ContextID id,
    EncryptedBlobLoad* load,
    uint8_t* bytes,
    uint64_t size,
    PlaintextParser consume
) {
    // Preserve original ciphertext; parse_blob verifies its hash when transfer completes.
    int result = parse_blob(cli, id, &load->transfer, bytes, size);
    if (result != CLIENT_OK && result != CLIENT_PARSE_DONE) {
        return result;
    }

    Buffer* scratch = &cli->net.recv_buffers[id];
    // Every reply carries a hash; only the first adds the whole-blob size.
    const uint8_t* payload = bytes + HASH_SIZE;
    size_t payload_size = size - HASH_SIZE;
    if (!load->started) {
        StoreItem* item = store_get_item(&cli->blob_store, &load->transfer.hash);
        load->reader.remaining = item->size;
        load->started = true;

        uint64_t capacity = CRYPT_RECORD_MAX + ENCRYPTED_CHUNK_OVERHEAD;
        if (capacity > item->size) {
            capacity = item->size;
        }
        if (buffer_ensure_min_cap(cli, scratch, capacity) != CLIENT_OK) {
            return CLIENT_ERR;
        }

        // A cached response may carry just the hash; open the already verified stored bytes.
        if (result == CLIENT_PARSE_DONE) {
            payload = item->b;
            payload_size = item->size;
        } else {
            payload += sizeof(uint64_t);
            payload_size -= sizeof(uint64_t);
        }
    }

    // A reply can contain several encrypted chunks or end partway through one.
    for (size_t offset = 0; offset < payload_size;) {
        size_t consumed;
        int parsed = parse_encrypted_blob(
            &load->reader, &load->key, scratch,
            payload + offset, payload_size - offset, &consumed
        );
        if (parsed == ENCRYPTED_BLOB_ERR) {
            return CLIENT_ERR;
        }
        offset += consumed;

        // Only authenticated plaintext reaches the client-selected content consumer.
        if (parsed != ENCRYPTED_BLOB_MORE &&
            consume(cli, id, scratch->b, scratch->size) != CLIENT_OK) {
            return CLIENT_ERR;
        }
    }

    if (result == CLIENT_PARSE_DONE &&
        finish_encrypted_blob(&load->reader) != ENCRYPTED_BLOB_DONE) {
        return CLIENT_ERR;
    }
    // The operation still has to finish its content parser before publishing the result.
    return result;
}
