/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz/utils/pqueue.h"
#include <stdlib.h>

static void pq_swap(PQNode *a, PQNode *b)
{
    PQNode tmp = *a;
    *a = *b;
    *b = tmp;
}

int pq_init(PriorityQueue *pq, size_t initial_capacity)
{
    if (initial_capacity == 0)
        initial_capacity = 16;

    pq->nodes = malloc(initial_capacity * sizeof(PQNode));
    if (!pq->nodes)
        return 0;

    pq->len = 0;
    pq->capacity = initial_capacity;

    return 1;
}

void pq_destroy(PriorityQueue *pq)
{
    free(pq->nodes);

    pq->nodes = NULL;
    pq->len = 0;
    pq->capacity = 0;
}

static int pq_grow(PriorityQueue *pq)
{
    size_t new_capacity = pq->capacity * 2;

    PQNode *new_nodes = realloc(
        pq->nodes,
        new_capacity * sizeof(PQNode)
    );

    if (!new_nodes)
        return 0;

    pq->nodes = new_nodes;
    pq->capacity = new_capacity;

    return 1;
}

int pq_push(PriorityQueue *pq, uint64_t priority, HashT *h)
{
    if (pq->len == pq->capacity) {
        if (!pq_grow(pq))
            return 0;
    }

    size_t i = pq->len++;

    pq->nodes[i] = (PQNode) {
        .priority = priority,
        .h = *h
    };

    /* Bubble upward. */
    while (i > 0) {
        size_t parent = (i - 1) / 2;

        // if parent prio is less than this stop
        if (pq->nodes[parent].priority <= pq->nodes[i].priority)
            break;

        pq_swap(&pq->nodes[parent], &pq->nodes[i]);

        i = parent;
    }

    return 1;
}

PQNode *pq_peek(PriorityQueue *pq)
{
    if (pq->len == 0)
        return NULL;

    return &pq->nodes[0];
}

PQNode pq_pop(PriorityQueue *pq)
{
    PQNode result = {0};

    if (pq->len == 0)
        return result;

    result = pq->nodes[0];

    pq->len--;

    if (pq->len == 0)
        return result;

    pq->nodes[0] = pq->nodes[pq->len];

    /* Bubble downward. */
    size_t i = 0;

    while (1) {
        size_t left = i * 2 + 1;
        size_t right = i * 2 + 2;
        size_t smallest = i;

        if (left < pq->len &&
            pq->nodes[left].priority < pq->nodes[smallest].priority)
            smallest = left;

        if (right < pq->len &&
            pq->nodes[right].priority < pq->nodes[smallest].priority)
            smallest = right;

        if (smallest == i)
            break;

        pq_swap(&pq->nodes[i], &pq->nodes[smallest]);

        i = smallest;
    }

    return result;
}

size_t pq_size(const PriorityQueue *pq) {
    return pq->len;
}

int pq_is_empty(const PriorityQueue *pq) {
    return pq->len == 0;
}
