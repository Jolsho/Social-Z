/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#undef realloc
#include "utils/store.h"
#include <assert.h>
#include <stdlib.h>

static int fail_realloc;

/* The lifecycle target redirects queue growth here to exercise allocation failure. */
void* store_test_realloc(void* memory, size_t size)
{
    return fail_realloc ? NULL : realloc(memory, size);
}

static void assert_empty(const Store* store)
{
    assert(!store->table.nodes && !store->table._b && !store->pq.nodes);
    assert(!store->mem && !store->mem_max && !store->counter);
    assert(!store->callback.destroy && !store->callback.ctx);
}

static void test_setup_and_destroy(void)
{
    Store* store = malloc(sizeof(*store));
    assert(store);
    memset(store, 0xa5, sizeof(*store));
    assert(store_setup(store, 200000) == STORE_OK);
    assert(store->mem == 0 && store->counter == 0 && pq_size(&store->pq) == 0);
    assert(store->table.val_c == &store->callback);
    assert(store->callback.ctx == store && store->callback.destroy);
    assert(store->mem_max < 200000 && store->mem_max > 100000);

    BufferPool pool;
    assert(buffer_pool_init(&pool, 64, 4, 256, 1, 4096, 1) == 0);
    for (uint8_t id = 1; id <= 3; id++) {
        HashT key = {0};
        key.b[0] = id;
        size_t size = 64;
        uint8_t* bytes = buffer_pool_pop(&pool, &size);
        assert(bytes);
        memset(bytes, id, size);
        assert(store_assign_item(store, &key, bytes, size, &pool));
        StoreItem* item = store_get_item(store, &key);
        assert(item && item->b == bytes && item->size == size && item->b[0] == id);
    }
    assert(store->mem == 192 && pool.buckets[0].available == 1);
    store_destroy(store);
    assert_empty(store);
    assert(pool.buckets[0].available == 4);
    store_destroy(store);
    assert_empty(store);
    assert(store_setup(store, 200000) == STORE_OK);
    store_destroy(store);
    buffer_pool_destroy(&pool);
    free(store);
}

static void test_invalid_budgets(void)
{
    const uint64_t budgets[] = {0, 1, 9999, 100000};
    for (size_t i = 0; i < sizeof(budgets) / sizeof(budgets[0]); i++) {
        Store store;
        memset(&store, 0xa5, sizeof(store));
        assert(store_setup(&store, budgets[i]) == STORE_ERR);
        assert_empty(&store);
        store_destroy(&store);
    }
    assert(store_setup(NULL, 200000) == STORE_ERR);
    store_destroy(NULL);
}

static void test_pool_ownership(void)
{
    Store store;
    BufferPool pool;
    HashT key = {0};
    assert(store_setup(&store, 200000) == STORE_OK);
    assert(buffer_pool_init(&pool, 64, 3, 256, 1, 4096, 1) == 0);
    size_t capacity = 7;
    uint8_t* first = buffer_pool_pop(&pool, &capacity);
    assert(first && capacity == 64);

    /* A response view, excess length, or excess allocation budget must not be retained. */
    assert(!store_assign_item(&store, &key, first + 1, 7, &pool));
    assert(!store_assign_item(&store, &key, first, 65, &pool));
    store.mem_max = 32;
    assert(!store_assign_item(&store, &key, first, 7, &pool));
    assert(!store.mem && !store.table.size && !pq_size(&store.pq));
    store.mem_max = 64;
    StoreItem* item = store_assign_item(&store, &key, first, 7, &pool);
    assert(item && item->size == 7 && item->capacity == 64 && store.mem == 64);

    capacity = 9;
    uint8_t* second = buffer_pool_pop(&pool, &capacity);
    assert(second);
    item = store_assign_item(&store, &key, second, 9, &pool);
    assert(item && item->b == second && item->size == 9);
    assert(store.mem == 64 && pool.buckets[0].available == 2);
    store_erase_item(&store, &key);
    assert(!store.mem && pool.buckets[0].available == 3);
    store_erase_item(&store, &key);
    assert(!store.mem && pool.buckets[0].available == 3);

    capacity = 7;
    first = buffer_pool_pop(&pool, &capacity);
    assert(first && store_assign_item(&store, &key, first, 7, &pool));
    HashT other = {.b = {1}};
    capacity = 9;
    second = buffer_pool_pop(&pool, &capacity);
    assert(second && store_assign_item(&store, &other, second, 9, &pool));
    assert(!ht_lookup(&store.table, &key) && ht_lookup(&store.table, &other));
    assert(store.mem == 64 && pool.buckets[0].available == 2);
    store_destroy(&store);
    assert(pool.buckets[0].available == 3);
    buffer_pool_destroy(&pool);
}

static void test_full_table_and_heap_ownership(void)
{
    Store store;
    assert(store_setup(&store, 200000) == STORE_OK);
    size_t count = store.table.cap;
    for (size_t i = 0; i < count; i++) {
        HashT key = {0};
        memcpy(key.b, &i, sizeof(i));
        uint8_t* bytes = malloc(1);
        assert(bytes && store_assign_item(&store, &key, bytes, 1, NULL));
    }
    size_t queued = pq_size(&store.pq);
    uint64_t counter = store.counter;
    HashT missing = {0};
    memcpy(missing.b, &count, sizeof(count));
    uint8_t* rejected = malloc(1);
    assert(rejected);
    assert(!store_assign_item(&store, &missing, rejected, 1, NULL));
    assert(store.table.size == count && store.mem == count);
    assert(pq_size(&store.pq) == queued && store.counter == counter);
    *rejected = 42; /* Failed insertion leaves ownership with the caller. */
    free(rejected);

    HashT first = {0};
    uint8_t* replacement = malloc(3);
    assert(replacement && store_assign_item(&store, &first, replacement, 3, NULL));
    assert(store.table.size == count && store.mem == count + 2);
    store_erase_item(&store, &first);
    assert(store.mem == count - 1);
    store_destroy(&store);
}

static void test_queue_failure(void)
{
    Store store;
    HashT key = {0}, other = {.b = {1}};
    assert(store_setup(&store, 200000) == STORE_OK);
    uint8_t* original = malloc(1);
    uint8_t* candidate = malloc(2);
    assert(original && candidate);
    assert(store_assign_item(&store, &key, original, 1, NULL));
    uint64_t counter = store.counter;
    store.pq.capacity = store.pq.len;
    fail_realloc = 1;
    assert(!store_assign_item(&store, &other, candidate, 2, NULL));
    assert(!ht_lookup(&store.table, &other));
    assert(!store_assign_item(&store, &key, candidate, 2, NULL));
    StoreItem* item = ht_lookup(&store.table, &key);
    assert(item && item->b == original && item->size == 1);
    assert(store.table.size == 1 && store.mem == 1 && pq_size(&store.pq) == 1);
    assert(store.counter == counter);
    fail_realloc = 0;
    assert(store_assign_item(&store, &key, candidate, 2, NULL));
    assert(store.mem == 2);
    store_destroy(&store);
}

int main(void)
{
    test_setup_and_destroy();
    test_invalid_budgets();
    test_pool_ownership();
    test_full_table_and_heap_ownership();
    test_queue_failure();
    return 0;
}
