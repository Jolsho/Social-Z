/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "utils/store.h"
#include "utils/store_internal.h"
#include "sz_common/hashtable.h"
#include "sz_common/pqueue.h"

int compare_pqnode(void* n1, void* n2) {
    uint64_t left = ((PQNode*)n1)->priority;
    uint64_t right = ((PQNode*)n2)->priority;
    return (left > right) - (left < right);
}

void _destroy_store_item_callback(void* ctx, void* itemp) {
    StoreItem* si = itemp;
    if (si->b && buffer_pool_push(si->pool, si->b, si->size) < 0) {
        free(si->b);
    }
    memset(si, 0, sizeof(StoreItem));
}

int store_setup(Store* s, uint64_t max_memory) {
    assert(s != NULL);

    uint64_t NODE_COUNT = floor(max_memory * 0.0001);
    if (NODE_COUNT > 1024) NODE_COUNT = 1024;

    // subtract estimated hash table overhead
    max_memory -= ht_memory_overhead(sizeof(StoreItem), NODE_COUNT);

    assert(max_memory > 100000);   // must be over a 100kb
    // if you are lower than that you can't hold modern store anyway.

    s->mem_max = max_memory;

    FreeValueCallback val_callback = { 
        .ctx = s, 
        .destroy = _destroy_store_item_callback 
    };

    ht_setup(&s->table, &val_callback, sizeof(StoreItem), NODE_COUNT);

    pq_init(&s->pq, sizeof(PQNode), 2048, compare_pqnode);

    return STORE_OK;
}



StoreItem* store_assign_item(
    Store* s, HashT* h, 
    uint8_t* b, uint64_t size,
    BufferPool* pool
) {
    if (size > s->mem_max) return NULL;

    if (s->counter == 0) s->counter = UINT64_MAX / 2;


    uint64_t new_prio = s->counter++;

    PQNode n;
    memcpy(&n.h, h, HASH_SIZE);
    n.priority = new_prio;
    
    if (pq_push(&s->pq, &n) == 0) return NULL;

    StoreItem* si = ht_reserve(&s->table, h);
    if (!si) return NULL;

    si->priority = new_prio;
    si->pool = pool;

    si->b = b;
    si->size = size;

    s->mem += size;
    store_evict(s);

    return si;
}

StoreItem* store_get_item(Store* s, HashT* h) {
    StoreItem* si = HT_LOOKUP_AS(StoreItem, &s->table, h);
    if (!si) return NULL;

    if (s->counter == 0) s->counter = UINT64_MAX / 2;
    uint64_t new_prio = s->counter++;

    PQNode n;
    memcpy(&n.h, h, HASH_SIZE);
    n.priority = new_prio;

    if (pq_push(&s->pq, &n) == 1) {
        si->priority = new_prio;
    }

    return si;
}

int store_copy_too_item(Store* s, HashT* h, uint64_t offset, uint8_t* b, uint64_t* l) {
    assert(s != NULL && h != NULL && b != NULL && l != NULL);

    StoreItem* item = ht_lookup(&s->table, h);
    if (!item) return STORE_NOT_EXIST;

    if (offset >= item->size) {
        store_erase_item(s, h);
        return STORE_ERR;
    }

    uint64_t remaining = item->size - offset;
    if (*l > remaining) *l = remaining;

    memcpy(item->b + offset, b, *l);
    offset += *l;

    if (offset == item->size) return STORE_DONE;

    return STORE_OK;
}

int store_copy_from_item(
    Store* s, HashT* h, uint64_t offset,
    uint8_t* b, uint64_t* l
) {

    StoreItem* item = ht_lookup(&s->table, h);
    if (!item) return STORE_NOT_EXIST;

    if (offset >= item->size) {
        store_erase_item(s, h);
        return STORE_ERR;
    }

    uint64_t remaining = item->size - offset;

    if (*l > remaining) *l = remaining;

    memcpy(b, item->b + offset, *l);
    offset += *l;

    if (offset == item->size) return STORE_DONE;

    return STORE_OK;
}

void store_evict(Store* s) {
    while (s->mem > s->mem_max && !pq_is_empty(&s->pq)) {
        PQNode node;
        if (!pq_pop(&s->pq, &node)) break;

        StoreItem* item = HT_LOOKUP_AS(StoreItem, &s->table, &node.h);
        if (!item || item->priority != node.priority) continue;

        s->mem -= item->size;
        store_erase_item(s, &node.h);
    }
}
