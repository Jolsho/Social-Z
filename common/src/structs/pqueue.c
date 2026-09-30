/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/pqueue.h"
#include <string.h>
#include <stdlib.h>

static void pq_swap(uint8_t *a, uint8_t *b, size_t node_size)
{
    while (node_size--) {
        unsigned char tmp = *a;
        *a++ = *b;
        *b++ = tmp;
    }
}

int pq_init(PriorityQueue *pq, size_t size_of_node, size_t initial_capacity, CompareNode cmp)
{
    if (initial_capacity == 0)
        initial_capacity = 16;

    pq->nodes = malloc(initial_capacity * size_of_node);
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

    uint8_t *new_nodes = realloc(
        pq->nodes,
        new_capacity * pq->size_of_node
    );

    if (!new_nodes) return 0;

    pq->nodes = new_nodes;
    pq->capacity = new_capacity;

    return 1;
}

int pq_push(PriorityQueue *pq, void* node)
{
    if (pq->len == pq->capacity) {
        if (!pq_grow(pq))
            return 0;
    }

    size_t i = pq->len++;

    memcpy(pq->nodes + (i * pq->size_of_node), node, pq->size_of_node);

    uint8_t tmp[pq->size_of_node];

    /* Bubble upward. */
    while (i > 0) {
        size_t parent = (i - 1) / 2;

        uint8_t* p_node = pq->nodes + (parent * pq->size_of_node);
        uint8_t* node = pq->nodes + (i * pq->size_of_node);

        // if parent prio is less than this stop
        if (pq->cmp(p_node, node) <= 0)
            break;

        pq_swap(p_node, node, pq->size_of_node);

        i = parent;
    }

    return 1;
}

void* pq_peek(PriorityQueue *pq)
{
    if (pq->len == 0)
        return NULL;
    return pq->nodes;
}

int pq_pop(PriorityQueue *pq, void* node)
{
    if (pq->len == 0)
        return 0;

    memcpy(node, pq->nodes, pq->size_of_node);

    pq->len--;

    if (pq->len == 0) return 1;

    pq->nodes[0] = pq->nodes[pq->len];

    /* Bubble downward. */
    size_t i = 0;

    while (1) {
        size_t left = i * 2 + 1;
        size_t right = i * 2 + 2;
        size_t smallest = i;

        if (left < pq->len &&
            pq->cmp(pq->nodes + (left * pq->size_of_node), 
                    pq->nodes + (smallest * pq->size_of_node)
                    ) < 0)
            smallest = left;

        if (right < pq->len &&
            pq->cmp(pq->nodes + (right * pq->size_of_node), 
                    pq->nodes + (smallest * pq->size_of_node)
                    ) < 0)
            smallest = right;

        if (smallest == i)
            break;

        pq_swap(pq->nodes + (i * pq->size_of_node), pq->nodes + (smallest * pq->size_of_node), pq->size_of_node);

        i = smallest;
    }

    return 1;
}

size_t pq_size(const PriorityQueue *pq) {
    return pq->len;
}

int pq_is_empty(const PriorityQueue *pq) {
    return pq->len == 0;
}
