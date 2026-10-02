/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "client.h"
#include "netwrk/context.h"
#include "sz_common/hash.h"
#include <stdlib.h>

int parse_blob(struct Client* cli, ContextID id, uint8_t* b, uint64_t len) {
    if (!valid_id(id)) return CLIENT_INVALID_ID;
    if (!cli || !b) return CLIENT_ERR;
    if (len < HASH_SIZE) return CLIENT_SMALL_BUFFER;
    HashT h;
    memcpy(&h, b, HASH_SIZE);
    b += HASH_SIZE;
    len -= HASH_SIZE;
    ConState* state = &cli->net.states[id];
    if (state->blob_active && !hash_is_equal(&state->blob_hash, &h)) return CLIENT_ERR;
    StoreItem* item = store_get_item(&cli->blob_store, &h);
    if (item && item->received == item->size) {
        state->blob_active = false;
        return CLIENT_PARSE_DONE;
    }
    if (item && (!state->blob_active || item->context != id)) return CLIENT_CONN_BUSY;
    if (!item) {
        if (state->blob_active) {
            state->blob_active = false;
            return CLIENT_ERR;
        }
        if (len < sizeof(uint64_t)) return CLIENT_SMALL_BUFFER;
        uint64_t size;
        memcpy(&size, b, sizeof(size));
        b += sizeof(size);
        len -= sizeof(size);
        if (!size || size > SIZE_MAX || size > cli->blob_store.mem_max || len > size)
            return CLIENT_ERR;
        BufferPool* pool = size <= cli->pool.buckets[BUFFER_BUCKETS - 1].buffer_size
                         ? &cli->pool : NULL;
        size_t capacity = (size_t)size;
        uint8_t* owned = pool ? buffer_pool_pop(pool, &capacity) : malloc(capacity);
        if (!owned) return CLIENT_ERR;
        item = store_assign_item(&cli->blob_store, &h, owned, size, pool);
        if (!item) {
            if (pool) buffer_pool_push(pool, owned, capacity);
            else free(owned);
            return CLIENT_ERR;
        }
        item->received = 0;
        item->context = id;
        state->blob_hash = h;
        state->blob_active = true;
    } else if (!len) {
        return CLIENT_SMALL_BUFFER;
    }
    if (len > item->size - item->received) {
        store_erase_item(&cli->blob_store, &h);
        state->blob_active = false;
        return CLIENT_ERR;
    }
    memcpy(item->b + item->received, b, (size_t)len);
    item->received += len;
    if (item->received < item->size) return CLIENT_OK;

    // All chunks are assembled; hash the entire blob, not this response's payload.
    Hasher hasher = new_hasher();
    hash_update(&hasher, item->b, (size_t)item->size);
    HashT actual = hash_finalize(&hasher);
    state->blob_active = false;
    if (!hash_is_equal(&h, &actual)) {
        store_erase_item(&cli->blob_store, &h);
        return CLIENT_ERR;
    }
    return CLIENT_PARSE_DONE;
}
