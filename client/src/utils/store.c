/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
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
    Store* s = ctx;
    StoreItem* si = itemp;
    if (si->b) {
        s->mem -= si->capacity;
        if (si->pool) buffer_pool_push(si->pool, si->b, si->capacity);
        else free(si->b);
    }
    memset(si, 0, sizeof(StoreItem));
}

int store_setup(Store* s, uint64_t max_memory) {
    if (!s) return STORE_ERR;
    memset(s, 0, sizeof(*s));
    size_t node_count = max_memory / 10000 > 1024 ? 1024 : (size_t)(max_memory / 10000);
    size_t overhead = ht_memory_overhead(sizeof(StoreItem), node_count);
    if (overhead == SIZE_MAX || max_memory <= overhead || max_memory - overhead <= 100000)
        return STORE_ERR;

    s->callback = (FreeValueCallback){
        .ctx = s, 
        .destroy = _destroy_store_item_callback 
    };
    if (ht_setup(&s->table, &s->callback, sizeof(StoreItem), node_count) != HT_SUCCESS)
        goto fail;
    if (!pq_init(&s->pq, sizeof(PQNode), 2048, compare_pqnode))
        goto fail;
    s->mem_max = max_memory - overhead;
    return STORE_OK;

fail:
    store_destroy(s);
    return STORE_ERR;
}

void store_destroy(Store* s) {
    if (!s) return;
    if (ht_is_initialized(&s->table)) ht_destroy(&s->table);
    pq_destroy(&s->pq);
    memset(s, 0, sizeof(*s));
}

static uint64_t pool_capacity(BufferPool* pool, uint8_t* b) {
    for (size_t i = 0; i < BUFFER_BUCKETS; i++) {
        BufferBucket* bucket = &pool->buckets[i];
        uintptr_t base = (uintptr_t)bucket->memory;
        uintptr_t address = (uintptr_t)b;
        if (!base || address < base || !bucket->buffer_size) continue;
        uintptr_t offset = address - base;
        if (offset / bucket->buffer_size < bucket->capacity &&
            offset % bucket->buffer_size == 0) return bucket->buffer_size;
    }
    return 0;
}

StoreItem* store_assign_item(
    Store* s, HashT* h, 
    uint8_t* b, uint64_t size,
    BufferPool* pool
) {
    if (!s || !h || !b || !size || !ht_is_initialized(&s->table)) return NULL;
    uint64_t capacity = pool ? pool_capacity(pool, b) : size;
    if (capacity < size || capacity > s->mem_max) return NULL;
    StoreItem* old = ht_lookup(&s->table, h);
    if (old && old->b == b) return NULL;
    uint64_t retained = s->mem - (old ? old->capacity : 0);
    if (capacity > UINT64_MAX - retained) return NULL;

    StoreItem* si = old ? old : ht_reserve(&s->table, h);
    if (!si) return NULL;
    if (!old) memset(si, 0, sizeof(*si));

    uint64_t priority = s->counter ? s->counter : UINT64_MAX / 2;
    PQNode n = {.priority = priority, .h = *h};
    if (!pq_push(&s->pq, &n)) {
        if (!old) ht_erase(&s->table, h);
        return NULL;
    }
    if (old) _destroy_store_item_callback(s, old);
    *si = (StoreItem){.b = b, .size = size, .capacity = capacity, .received = size,
                      .pool = pool, .priority = priority};
    s->counter = priority + 1;
    s->mem += capacity;
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
    if (item->received != item->size) return STORE_ERR;

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

        store_erase_item(s, &node.h);
    }
}
