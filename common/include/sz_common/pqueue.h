/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stdint.h>
#include <stddef.h>

typedef int (*CompareNode) (void* n1, void* n2);

typedef struct {
    size_t      size_of_node;
    CompareNode cmp;
    uint8_t*    nodes;
    size_t  len;
    size_t  capacity;
} PriorityQueue;

/* Initialize a priority queue. */
int pq_init(PriorityQueue *pq, size_t size_of_node, size_t initial_capacity, CompareNode cmp);

/* Free the queue's internal storage. */
void pq_destroy(PriorityQueue *pq);

/* Insert an item. Returns 1 on success, 0 on allocation failure. */
int pq_push(PriorityQueue *pq, void* node);

/* Return the highest-priority item without removing it. */
void *pq_peek(PriorityQueue *pq);

/* Remove and return the highest-priority item. */
int pq_pop(PriorityQueue *pq, void* node);

/* Return the number of items in the queue. */
size_t pq_size(const PriorityQueue *pq);

/* Return non-zero if the queue is empty. */
int pq_is_empty(const PriorityQueue *pq);

#endif /* PRIORITY_QUEUE_H */
