/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "utils/buffers.h"
#include "sz_common/codec.h"
#include "sz_common/hashtable.h"
#include "sz_common/pqueue.h"
#ifdef __cplusplus
extern "C" {
#endif

#define STORE_DONE      1
#define STORE_OK        0
#define STORE_ERR       -1
#define STORE_NOT_EXIST -2
#define STORE_INTERNAL  -3

typedef struct Store {
    HashTable       table;
    PriorityQueue   pq;
    uint64_t        counter;
    uint64_t        mem;
    uint64_t        mem_max;
    FreeValueCallback callback;
} Store;

typedef struct StoreItem {
    uint8_t*    b;
    uint64_t    size;
    uint64_t    capacity;
    uint64_t    received; // Partial blobs are readable only when this reaches size.
    int16_t     context; // Receiving context while the blob is incomplete.
    BufferPool* pool;
    uint64_t    priority;
} StoreItem;

// Initializes a fresh store; destroy it before setting it up again.
int store_setup(Store* s, uint64_t max_memory);

// Takes ownership of uint8_t *b on success; the caller keeps ownership on failure.
// b must be owned by the caller: a pool buffer start, or malloc memory when pool is NULL.
// The budget counts pool capacity, or size for malloc memory.
// A supplying pool must outlive its cached buffers.
StoreItem* store_assign_item(
    Store* s, HashT* h, 
    uint8_t* b, uint64_t size,
    BufferPool* pool
);
void store_destroy(Store* s);
StoreItem* store_get_item(Store* s, HashT* h);
static inline void store_erase_item(Store* s, HashT* h) { ht_erase(&s->table, h); }
int store_copy_too_item(Store* s, HashT* h, uint64_t offset, uint8_t* b, uint64_t* l);
int store_copy_from_item(Store* s, HashT* h, uint64_t offset, uint8_t* b, uint64_t* l);
void store_evict(Store* s);

#ifdef __cplusplus
}
#endif
