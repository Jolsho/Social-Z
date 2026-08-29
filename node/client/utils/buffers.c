
/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "buffers.h"


typedef struct BufferNode {
    struct BufferNode *next;
} BufferNode;


/*
 * Initialize a bucket.
 *
 * All buffers are allocated as one contiguous block, and the free list
 * is constructed over that block.
 */
static int
buffer_bucket_init(BufferBucket *bucket, size_t buffer_size, size_t capacity) {
    bucket->buffer_size = buffer_size;
    bucket->capacity = capacity;
    bucket->available = capacity;

    bucket->memory = malloc(buffer_size * capacity);
    if (!bucket->memory)
        return -1;

    bucket->free_list = NULL;

    uint8_t *memory = bucket->memory;

    for (size_t i = 0; i < capacity; ++i) {
        BufferNode *node =
            (BufferNode *)(memory + i * buffer_size);

        node->next = bucket->free_list;
        bucket->free_list = node;
    }

    return 0;
}


int
buffer_pool_init(BufferPool *pool,
                 size_t size0, size_t capacity0,
                 size_t size1, size_t capacity1,
                 size_t size2, size_t capacity2)
{
    memset(pool, 0, sizeof(*pool));

    if (buffer_bucket_init(&pool->buckets[0], size0, capacity0) != 0)
        goto fail;

    if (buffer_bucket_init(&pool->buckets[1], size1, capacity1) != 0)
        goto fail;

    if (buffer_bucket_init(&pool->buckets[2], size2, capacity2) != 0)
        goto fail;

    return 0;

fail:
    for (size_t i = 0; i < BUFFER_BUCKETS; ++i)
        free(pool->buckets[i].memory);

    memset(pool, 0, sizeof(*pool));
    return -1;
}


uint8_t *buffer_pool_pop(BufferPool *pool, size_t* size)
{
    /*
     * Find the smallest bucket that can hold `size`.
     */
    for (size_t i = 0; i < BUFFER_BUCKETS; ++i) {
        BufferBucket *bucket = &pool->buckets[i];

        if (bucket->buffer_size < *size)
            continue;

        *size = bucket->buffer_size;

        if (!bucket->free_list)
            continue;

        BufferNode *node = bucket->free_list;

        bucket->free_list = node->next;
        bucket->available--;

        return (uint8_t*)node;
    }

    return NULL;
}


int
buffer_pool_push(BufferPool *pool, void *buffer, size_t size)
{
    /*
     * `size` is the bucket size, not necessarily the original
     * requested size.
     */
    for (size_t i = 0; i < BUFFER_BUCKETS; ++i) {
        BufferBucket *bucket = &pool->buckets[i];

        if (bucket->buffer_size != size)
            continue;

        BufferNode *node = buffer;

        node->next = bucket->free_list;
        bucket->free_list = node;
        bucket->available++;

        return 0;
    }

    return -1;
}


void
buffer_pool_destroy(BufferPool *pool) {
    for (size_t i = 0; i < BUFFER_BUCKETS; ++i) {
        free(pool->buckets[i].memory);
        pool->buckets[i].memory = NULL;
        pool->buckets[i].free_list = NULL;
    }
}
