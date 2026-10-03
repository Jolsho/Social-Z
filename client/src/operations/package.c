/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "operations/package.h"
#include "operations/blob.h"
#include "codec/blob.h"
#include "codec/encrypted_blob_reader.h"
#include "codec/package.h"
#include "networking/context.h"
#include "networking/dispatch.h"
#include <sodium.h>
#include <stdlib.h>

typedef struct PackageLoad {
    // One instance per context, retained across replies until its cleanup handler runs.
    BlobTransfer transfer;
    Key key;
    bool started; // The first transfer reply supplies the total ciphertext size.

    // reader opens encrypted chunks; parser interprets their package plaintext.
    EncryptedBlobReader reader;
    PackageParser parser;

    // Staging owns provisional contents. destination belongs to the caller.
    Package staging;
    Package* destination;
} PackageLoad;

void package_cleanup(struct Client* cli, ContextID id) {
    PackageLoad* load = cli->net.states[id].operation;
    if (load) {
        // Keep completed ciphertext cached; discard partial transfer and provisional plaintext.
        blob_discard_partial(cli, id, &load->transfer);
        package_destroy(&load->staging);
        sodium_memzero(load, sizeof(*load));
        free(load);
    }

    Buffer* scratch = &cli->net.recv_buffers[id];
    if (scratch->b) {
        // This networking buffer held plaintext and will be returned to the shared pool.
        sodium_memzero(scratch->b, scratch->cap);
    }
}

int package_get(
    struct Client* cli,
    const Key* owner,
    const HashT* label,
    const Key* key,
    Package* destination,
    size_t max_size,
    ContextID* id
) {
    if (!cli || !owner || !label || !key || !destination || !max_size || !id) {
        return CLIENT_ERR;
    }

    ContextID context = client_new_context(&cli->net);
    if (!valid_id(context)) {
        return CLIENT_CONN_BUSY;
    }

    // Install the handler and its state before sending; the host may reply during send_request.
    ConState* state = &cli->net.states[context];
    state->parser_id = PARSER_ID_PACKAGE;
    PackageLoad* load = calloc(1, sizeof(*load));
    state->operation = load;
    if (!load) {
        client_free_context(cli, context);
        return CLIENT_ERR;
    }

    load->key = *key;
    load->destination = destination;
    load->parser.max_size = max_size;
    *id = context;
    return blob_send_get(cli, context, owner, label);
}

int package_handle_response(struct Client* cli, ContextID id, uint8_t* bytes, uint64_t size) {
    PackageLoad* load = cli->net.states[id].operation;
    // Cache the original ciphertext and verify its whole-blob hash when transfer completes.
    int result = parse_blob(cli, id, &load->transfer, bytes, size);
    if (result != CLIENT_OK && result != CLIENT_PARSE_DONE) {
        goto done;
    }

    Buffer* scratch = &cli->net.recv_buffers[id];
    // Every transport reply starts with the hash; only the first adds a total-size field.
    const uint8_t* payload = bytes + HASH_SIZE;
    size_t payload_size = size - HASH_SIZE;
    if (!load->started) {
        StoreItem* item = store_get_item(&cli->blob_store, &load->transfer.hash);
        // The encrypted reader uses this total to identify the chunk that must carry FINAL.
        load->reader.remaining = item->size;
        load->started = true;

        uint64_t capacity = CRYPT_RECORD_MAX + ENCRYPTED_CHUNK_OVERHEAD;
        if (capacity > item->size) {
            capacity = item->size;
        }
        if (buffer_ensure_min_cap(cli, scratch, capacity) != CLIENT_OK) {
            result = CLIENT_ERR;
            goto done;
        }

        // A cached response may contain just the hash; decrypt the already verified bytes.
        if (result == CLIENT_PARSE_DONE) {
            payload = item->b;
            payload_size = item->size;
        } else {
            payload += sizeof(uint64_t);
            payload_size -= sizeof(uint64_t);
        }
    }

    // Each authenticated slice is copied into staging before scratch is reused.
    // consumed advances through replies containing several encrypted chunks; split chunks stay pending.
    for (size_t offset = 0; offset < payload_size;) {
        size_t consumed;
        int parsed = parse_encrypted_blob(
            &load->reader, &load->key, scratch,
            payload + offset, payload_size - offset, &consumed
        );
        if (parsed == ENCRYPTED_BLOB_ERR) {
            result = CLIENT_ERR;
            goto done;
        }
        offset += consumed;
        // MORE only collected ciphertext; CHUNK/DONE provide usable plaintext.
        if (parsed != ENCRYPTED_BLOB_MORE && parse_package_contents(
            &load->parser, &load->staging, scratch->b, scratch->size
        ) == PACKAGE_ERR) {
            result = CLIENT_ERR;
            goto done;
        }
    }

    if (result == CLIENT_OK) {
        // Even if some plaintext is parsed, the complete ciphertext hash is still pending.
        return result;
    }
    if (finish_encrypted_blob(&load->reader) != ENCRYPTED_BLOB_DONE ||
        finish_package(&load->parser) != PACKAGE_DONE) {
        result = CLIENT_ERR;
        goto done;
    }

    // All parsing, final-tag checks, and whole-ciphertext hashing have succeeded.
    package_destroy(load->destination);
    *load->destination = load->staging;
    // Transfer ownership so context cleanup does not destroy the newly loaded package.
    load->staging = (Package){0};

done:
    client_free_context(cli, id);
    return result;
}
