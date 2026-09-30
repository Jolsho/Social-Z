/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/pqueue.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint64_t priority;
    uint64_t id;
    uint8_t payload[24];
} Record;

static int compare_record(void* a, void* b)
{
    const Record* left = a;
    const Record* right = b;
    return (left->priority > right->priority) - (left->priority < right->priority);
}

static Record make_record(uint64_t priority, uint64_t id)
{
    Record record = {.priority = priority, .id = id};
    for (size_t i = 0; i < sizeof(record.payload); i++)
        record.payload[i] = (uint8_t)(id * 17 + i);
    return record;
}

static void check_record(const Record* actual, const Record* expected)
{
    assert(actual->priority == expected->priority);
    assert(actual->id == expected->id);
    assert(memcmp(actual->payload, expected->payload, sizeof(actual->payload)) == 0);
}

static void check_empty(PriorityQueue* queue)
{
    Record output = make_record(123, 456);
    Record unchanged = output;
    assert(pq_is_empty(queue));
    assert(pq_size(queue) == 0);
    assert(pq_peek(queue) == NULL);
    assert(pq_pop(queue, &output) == 0);
    check_record(&output, &unchanged);
}

static void test_ordering_and_growth(size_t initial_capacity)
{
    PriorityQueue queue = {0};
    assert(pq_init(&queue, sizeof(Record), initial_capacity, compare_record) == 1);
    assert(queue.size_of_node == sizeof(Record));
    assert(queue.cmp == compare_record);
    check_empty(&queue);

    /* Visit all 40 priorities in a shuffled order and force repeated growth. */
    for (uint64_t i = 0; i < 40; i++) {
        uint64_t priority = (i * 13 + 7) % 40;
        Record record = make_record(priority, priority + 1000);
        assert(pq_push(&queue, &record) == 1);
        assert(pq_size(&queue) == i + 1);
    }

    for (uint64_t priority = 0; priority < 40; priority++) {
        Record expected = make_record(priority, priority + 1000);
        Record output;
        assert(!pq_is_empty(&queue));
        check_record(pq_peek(&queue), &expected);
        assert(pq_size(&queue) == 40 - priority);
        assert(pq_pop(&queue, &output) == 1);
        check_record(&output, &expected);
        assert(pq_size(&queue) == 39 - priority);
    }
    check_empty(&queue);

    /* An emptied queue can be reused, including the single-item pop path. */
    Record record = make_record(9, 2000);
    Record output;
    assert(pq_push(&queue, &record) == 1);
    check_record(pq_peek(&queue), &record);
    assert(pq_pop(&queue, &output) == 1);
    check_record(&output, &record);
    check_empty(&queue);
    pq_destroy(&queue);
}

static void test_equal_priorities(void)
{
    PriorityQueue queue = {0};
    assert(pq_init(&queue, sizeof(Record), 2, compare_record) == 1);
    for (uint64_t id = 0; id < 8; id++) {
        Record record = make_record(5, id);
        assert(pq_push(&queue, &record) == 1);
    }

    unsigned int seen = 0;
    for (size_t i = 0; i < 8; i++) {
        Record output;
        assert(pq_pop(&queue, &output) == 1);
        assert(output.id < 8);
        assert(!(seen & (1u << output.id)));
        Record expected = make_record(5, output.id);
        check_record(&output, &expected);
        seen |= 1u << output.id;
    }
    assert(seen == 0xff);
    check_empty(&queue);
    pq_destroy(&queue);
}

int main(void)
{
    test_ordering_and_growth(2);
    test_ordering_and_growth(0);
    test_equal_priorities();
    puts("Priority queue regression tests passed.");
    return 0;
}
