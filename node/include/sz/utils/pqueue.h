/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "sz/codec.h"
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint64_t priority;
    HashT   h;
} PQNode;

typedef struct {
    PQNode *nodes;
    size_t len;
    size_t capacity;
} PriorityQueue;

/* Initialize a priority queue. */
int pq_init(PriorityQueue *pq, size_t initial_capacity);

/* Free the queue's internal storage. */
void pq_destroy(PriorityQueue *pq);

/* Insert an item. Returns 1 on success, 0 on allocation failure. */
int pq_push(PriorityQueue *pq, uint64_t priority, HashT* h);

/* Return the highest-priority item without removing it. */
PQNode *pq_peek(PriorityQueue *pq);

/* Remove and return the highest-priority item. */
PQNode pq_pop(PriorityQueue *pq);

/* Return the number of items in the queue. */
size_t pq_size(const PriorityQueue *pq);

/* Return non-zero if the queue is empty. */
int pq_is_empty(const PriorityQueue *pq);

#endif /* PRIORITY_QUEUE_H */
