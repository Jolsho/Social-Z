/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "utils/store.h"
#include <assert.h>
#include <stdlib.h>

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

int main(void)
{
    test_setup_and_destroy();
    test_invalid_budgets();
    return 0;
}
