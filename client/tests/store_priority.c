/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "utils/store_internal.h"
#include "sz_common/pqueue.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    const uint64_t priorities[] = {
        0, 1, UINT64_C(1) << 31, UINT64_C(1) << 32,
        UINT64_MAX / 2, UINT64_MAX - 1, UINT64_MAX
    };
    const size_t count = sizeof(priorities) / sizeof(priorities[0]);
    for (size_t i = 0; i < count; i++) {
        for (size_t j = 0; j < count; j++) {
            PQNode left = {.priority = priorities[i]};
            PQNode right = {.priority = priorities[j]};
            int result = compare_pqnode(&left, &right);
            assert((result > 0) == (i > j));
            assert((result < 0) == (i < j));
            assert((result == 0) == (i == j));
        }
    }

    PriorityQueue queue = {0};
    assert(pq_init(&queue, sizeof(PQNode), 2, compare_pqnode));
    for (size_t i = count; i > 0; i--) {
        PQNode node = {.priority = priorities[i - 1]};
        assert(pq_push(&queue, &node));
    }
    for (size_t i = 0; i < count; i++) {
        PQNode node;
        assert(pq_pop(&queue, &node));
        assert(node.priority == priorities[i]);
    }
    pq_destroy(&queue);
    puts("Store priority tests passed.");
    return 0;
}
