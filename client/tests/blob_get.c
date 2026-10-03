/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "operations/blob.h"
#include "networking/context.h"
#include "sz_common/hash.h"
#include <assert.h>

static Buffer* pending;
static Request sent;

void send_request(struct Client* cli, ContextID id, struct Buffer* buffer) {
    assert(!pending && buffer == &cli->net.send_buffers[id]);
    assert(cli->net.states[id].send_owned);
    assert(parse_request(&sent, buffer->b, buffer->size) == 0);
    pending = buffer;
}

int main(void) {
    struct Client* cli = init_client();
    assert(cli);
    Key owner = {{1}};
    HashT label = {{2}};
    uint8_t payload[] = {3, 4, 5, 6};
    Hasher hasher = new_hasher();
    hash_update(&hasher, payload, sizeof(payload));
    HashT hash = hash_finalize(&hasher);

    ContextID id;
    assert(blob_get(cli, &owner, &label, &id) == CLIENT_OK);
    assert(sent.kind == REQUEST_BLOB_GET);
    assert(memcmp(sent.data.blob.owner.b, owner.b, KEY_SIZE) == 0);
    assert(hash_is_equal(&sent.data.blob.label, &label));
    uint8_t request[BLOB_GET_REQUEST_SIZE];
    memcpy(request, pending->b, sizeof(request));

    uint8_t chunk[HASH_SIZE + sizeof(uint64_t) + 2];
    memcpy(chunk, hash.b, HASH_SIZE);
    uint64_t size = sizeof(payload);
    memcpy(chunk + HASH_SIZE, &size, sizeof(size));
    memcpy(chunk + HASH_SIZE + sizeof(size), payload, 2);
    assert(client_parse_response(cli, id, chunk, sizeof(chunk)) == CLIENT_OK);
    memset(chunk, 0, sizeof(chunk));

    memcpy(chunk, hash.b, HASH_SIZE);
    memcpy(chunk + HASH_SIZE, payload + 2, 2);
    assert(client_parse_response(cli, id, chunk, HASH_SIZE + 2) == CLIENT_PARSE_DONE);
    memset(chunk, 0, sizeof(chunk));
    StoreItem* item = store_get_item(&cli->blob_store, &hash);
    assert(item && item->received == size && memcmp(item->b, payload, size) == 0);

    // Completion keeps the host's original buffer and context reserved until return.
    assert(cli->net.states[id].release_pending && cli->net.states[id].send_owned);
    assert(memcmp(pending->b, request, sizeof(request)) == 0);
    ContextID other = client_new_context(&cli->net);
    assert(valid_id(other) && other != id);
    client_free_context(cli, other);
    client_return_buffer(cli, pending);
    pending = NULL;
    assert(client_parse_response(cli, id, chunk, HASH_SIZE + 2) == CLIENT_ERR);

    // A bad continuation clears partial assembly without releasing the host's buffer.
    store_erase_item(&cli->blob_store, &hash);
    assert(blob_get(cli, &owner, &label, &id) == CLIENT_OK);
    memcpy(chunk, hash.b, HASH_SIZE);
    memcpy(chunk + HASH_SIZE, &size, sizeof(size));
    memcpy(chunk + HASH_SIZE + sizeof(size), payload, 2);
    assert(client_parse_response(cli, id, chunk, sizeof(chunk)) == CLIENT_OK);
    chunk[0] ^= 1;
    assert(client_parse_response(cli, id, chunk, HASH_SIZE + 2) == CLIENT_ERR);
    assert(!ht_lookup(&cli->blob_store.table, &hash));
    assert(cli->net.states[id].release_pending);
    client_return_buffer(cli, pending);
    pending = NULL;

    // The host can return the send buffer before any response arrives.
    assert(blob_get(cli, &owner, &label, &id) == CLIENT_OK);
    client_return_buffer(cli, pending);
    pending = NULL;
    assert(client_parse_response(cli, id, NULL, 0) == CLIENT_ERR);
    assert(cli->net.states[id].state == CON_DEAD);

    destroy_client(cli);
    return 0;
}
