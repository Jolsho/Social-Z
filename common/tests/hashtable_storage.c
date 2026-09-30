/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/hashtable.h"
#include <assert.h>

static HashT key_for(size_t id)
{
    HashT key = {0};
    memcpy(key.b, &id, sizeof(id));
    return key;
}

static void test_storage(size_t capacity, size_t value_size)
{
    HashTable table = {0};
    assert(ht_setup(&table, NULL, value_size, capacity) == HT_SUCCESS);
    assert(table._b == (uint8_t*)table.nodes);
    for (size_t i = 0; i < capacity; i++) {
        HashT key = key_for(i);
        uint8_t* value = ht_reserve(&table, &key);
        assert(value && (uintptr_t)value % _Alignof(max_align_t) == 0);
        for (size_t j = 0; j < value_size; j++) assert(value[j] == 0);
        memset(value, (int)(i % 255 + 1), value_size);
    }
    HashT extra = key_for(capacity);
    assert(ht_reserve(&table, &extra) == NULL);
    assert(table.size == capacity);
    for (size_t i = 0; i < capacity; i++) {
        HashT key = key_for(i);
        const uint8_t* value = ht_const_lookup(&table, &key);
        assert(value);
        for (size_t j = 0; j < value_size; j++) assert(value[j] == i % 255 + 1);
    }
    HashT first = key_for(0);
    assert(ht_erase(&table, &first) == HT_SUCCESS);
    uint8_t* reused = ht_reserve(&table, &extra);
    assert(reused);
    for (size_t j = 0; j < value_size; j++) assert(reused[j] == 0);
    assert(ht_clear(&table) == HT_SUCCESS);
    assert(table.size == 0 && ht_lookup(&table, &extra) == NULL);
    assert(ht_reserve(&table, &first));
    assert(ht_destroy(&table) == HT_SUCCESS);
    assert(!ht_is_initialized(&table));
    assert(!table._b && !table.pool && !table.value_pool && !table.free_list);
}

static void count_destroy(void* context, void* value)
{
    size_t* count = context;
    assert(*(uint64_t*)value != 0);
    ++*count;
}

static void test_live_destructors(void)
{
    HashTable table = {0};
    size_t destroyed = 0;
    FreeValueCallback callback = {.destroy = count_destroy, .ctx = &destroyed};
    assert(ht_setup(&table, &callback, sizeof(uint64_t), 4) == HT_SUCCESS);
    HashT first = key_for(1), second = key_for(2);
    *(uint64_t*)ht_reserve(&table, &first) = 1;
    *(uint64_t*)ht_reserve(&table, &second) = 2;
    assert(ht_erase(&table, &first) == HT_SUCCESS);
    assert(destroyed == 1);
    assert(ht_clear(&table) == HT_SUCCESS);
    assert(destroyed == 2);
    *(uint64_t*)ht_reserve(&table, &first) = 3;
    assert(ht_destroy(&table) == HT_SUCCESS);
    assert(destroyed == 3);
}

static void test_invalid_sizes(void)
{
    HashTable table = {0};
    assert(ht_setup(&table, NULL, 8, 0) == HT_ERROR);
    assert(ht_setup(&table, NULL, 0, 4) == HT_ERROR);
    assert(ht_setup(&table, NULL, SIZE_MAX, 1) == HT_ERROR);
    assert(ht_setup(&table, NULL, 8, SIZE_MAX) == HT_ERROR);
    assert(!ht_is_initialized(&table));
}

int main(void)
{
    test_storage(1, 3);
    test_storage(3, sizeof(max_align_t));
    test_storage(513, 17);
    test_live_destructors();
    test_invalid_sizes();
    return 0;
}
