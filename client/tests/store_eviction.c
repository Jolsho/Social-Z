/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "utils/store.h"
#include "utils/store_internal.h"

void _destroy_store_item_callback(void* ctx, void* itemp);

typedef struct {
    Store store;
    BufferPool buffers;
    HTNode* buckets[4];
    HTNode nodes[4];
    StoreItem items[4];
    FreeValueCallback callback;
} Fixture;

static HashT item_key(uint64_t id)
{
    HashT key = {0};
    memcpy(key.b, &id, sizeof(id));
    return key;
}

static void fixture_init(Fixture* fixture, uint64_t limit)
{
    memset(fixture, 0, sizeof(*fixture));
    assert(buffer_pool_init(&fixture->buffers, 256, 8, 4096, 1, 65536, 1) == 0);
    Store* store = &fixture->store;
    store->mem_max = limit;
    store->counter = 100;
    assert(pq_init(&store->pq, sizeof(PQNode), 2, compare_pqnode));

    /* Prepare table storage directly to isolate eviction from the existing
     * allocation and callback-lifetime defects in ht_setup/store_setup.
     * Lookup, insertion, erasure, the destructor, and the queue are real code.
     */
    fixture->callback = (FreeValueCallback){
        .destroy = _destroy_store_item_callback, .ctx = store
    };
    store->table.nodes = fixture->buckets;
    store->table.pool = fixture->nodes;
    store->table.value_size = sizeof(StoreItem);
    store->table.cap = store->table.base_cap = 4;
    store->table.val_c = &fixture->callback;
    store->table.free_list = fixture->nodes;
    for (size_t i = 0; i < 4; i++) {
        fixture->nodes[i].value = &fixture->items[i];
        fixture->nodes[i].next = i < 3 ? &fixture->nodes[i + 1] : NULL;
    }
}

static void fixture_add(Fixture* fixture, uint64_t id)
{
    Store* store = &fixture->store;
    HashT key = item_key(id);
    size_t size = 256;
    StoreItem item = {
        .b = buffer_pool_pop(&fixture->buffers, &size),
        .size = 256, .pool = &fixture->buffers, .priority = id
    };
    assert(item.b);
    assert(ht_insert(&store->table, &key, &item) == HT_INSERTED);
    PQNode node = {.priority = id, .h = key};
    assert(pq_push(&store->pq, &node));
    store->mem += item.size;
}

static StoreItem* fixture_lookup(Fixture* fixture, uint64_t id)
{
    HashT key = item_key(id);
    return ht_lookup(&fixture->store.table, &key);
}

static void fixture_destroy(Fixture* fixture)
{
    for (uint64_t id = 1; id <= 4; id++) {
        HashT key = item_key(id);
        if (ht_lookup(&fixture->store.table, &key))
            assert(ht_erase(&fixture->store.table, &key) == HT_SUCCESS);
    }
    assert(fixture->buffers.buckets[0].available == 8);
    pq_destroy(&fixture->store.pq);
    buffer_pool_destroy(&fixture->buffers);
}

static void test_budget_boundaries(void)
{
    Fixture fixture;
    fixture_init(&fixture, 512);
    fixture_add(&fixture, 1);
    store_evict(&fixture.store);
    assert(fixture.store.mem == 256);
    assert(pq_size(&fixture.store.pq) == 1);
    assert(fixture_lookup(&fixture, 1));

    fixture_add(&fixture, 2);
    store_evict(&fixture.store);
    assert(fixture.store.mem == 512);
    assert(pq_size(&fixture.store.pq) == 2);

    fixture_add(&fixture, 3);
    store_evict(&fixture.store);
    assert(fixture.store.mem == 512);
    assert(!fixture_lookup(&fixture, 1));
    assert(fixture_lookup(&fixture, 2));
    assert(fixture_lookup(&fixture, 3));
    assert(fixture.buffers.buckets[0].available == 6);
    fixture_destroy(&fixture);
}

static void test_refresh_and_missing_entries(void)
{
    Fixture fixture;
    fixture_init(&fixture, 512);
    fixture_add(&fixture, 1);
    fixture_add(&fixture, 2);
    fixture_add(&fixture, 3);
    HashT first = item_key(1);
    assert(store_get_item(&fixture.store, &first));
    uint64_t refreshed = fixture_lookup(&fixture, 1)->priority;
    store_evict(&fixture.store);
    assert(fixture.store.mem == 512);
    assert(fixture_lookup(&fixture, 1)->priority == refreshed);
    assert(!fixture_lookup(&fixture, 2));
    assert(fixture_lookup(&fixture, 3));

    HashT missing = item_key(4);
    size_t queued = pq_size(&fixture.store.pq);
    uint64_t counter = fixture.store.counter;
    assert(store_get_item(&fixture.store, &missing) == NULL);
    assert(pq_size(&fixture.store.pq) == queued);
    assert(fixture.store.counter == counter);

    /* Removed items leave stale queue entries which must be skipped. */
    HashT third = item_key(3);
    assert(ht_erase(&fixture.store.table, &third) == HT_SUCCESS);
    fixture.store.mem -= 256;
    fixture.store.mem_max = 0;
    store_evict(&fixture.store);
    assert(fixture.store.mem == 0);
    assert(!fixture_lookup(&fixture, 1));
    fixture_destroy(&fixture);
}

static void test_oversized_item(void)
{
    Fixture fixture;
    fixture_init(&fixture, 128);
    HashT key = item_key(1);
    size_t size = 256;
    uint8_t* bytes = buffer_pool_pop(&fixture.buffers, &size);
    assert(bytes);
    assert(store_assign_item(&fixture.store, &key, bytes, size, &fixture.buffers) == NULL);
    assert(fixture.store.mem == 0);
    assert(pq_is_empty(&fixture.store.pq));
    assert(!fixture_lookup(&fixture, 1));
    assert(fixture.buffers.buckets[0].available == 7);
    /* Rejection leaves the buffer owned by the caller. */
    assert(buffer_pool_push(&fixture.buffers, bytes, size) == 0);
    fixture_destroy(&fixture);
}

int main(void)
{
    test_budget_boundaries();
    test_refresh_and_missing_entries();
    test_oversized_item();
    puts("Store eviction tests passed.");
    return 0;
}
