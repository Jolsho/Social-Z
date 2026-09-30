/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/hashtable.h"
#include <assert.h>

int main(void)
{
    HTNode* buckets[2] = {0};
    uint64_t values[2] = {0};
    HTNode nodes[2] = {
        {.value = &values[0], .next = &nodes[1]},
        {.value = &values[1]}
    };
    /* Isolate reservation from the unfinished ht_setup allocator. */
    HashTable table = {
        .nodes = buckets, .pool = nodes, .free_list = nodes,
        .value_size = sizeof(values[0]), .cap = 2, .base_cap = 2
    };
    HashT first = {0};
    HashT second = {0};
    second.b[HASH_SIZE - 1] = 1; // Different keys in the same bucket.

    uint64_t* first_value = ht_reserve(&table, &first);
    assert(first_value == &values[0]);
    *first_value = 123;
    assert(table.size == 1);
    assert(ht_lookup(&table, &first) == first_value);
    assert(memcmp(nodes[0].key.b, first.b, HASH_SIZE) == 0);
    assert(nodes[0].value == first_value && nodes[0].next == NULL);

    assert(ht_reserve(&table, &first) == first_value);
    assert(table.size == 1 && *first_value == 123);

    uint64_t* second_value = ht_reserve(&table, &second);
    assert(second_value == &values[1]);
    *second_value = 456;
    assert(table.size == 2);
    assert(ht_lookup(&table, &first) == first_value);
    assert(ht_lookup(&table, &second) == second_value);
    assert(*first_value == 123 && *second_value == 456);

    assert(ht_erase(&table, &first) == HT_SUCCESS);
    assert(ht_lookup(&table, &first) == NULL);
    assert(*first_value == 0);
    assert(ht_reserve(&table, &first) == first_value);
    *first_value = 789;
    assert(ht_lookup(&table, &first) == first_value);
    assert(*second_value == 456);
    return 0;
}
