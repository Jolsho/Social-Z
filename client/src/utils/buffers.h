/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include <stdint.h>
#include <stddef.h>

#define BUFFER_BUCKETS 3

typedef struct BufferBucket {
    uint32_t buffer_size;
    uint32_t capacity;
    uint32_t available;

    void *memory;
    void *free_list;
} BufferBucket;

typedef struct BufferPool {
    BufferBucket buckets[BUFFER_BUCKETS];
} BufferPool;


/*
 * Initialize a buffer pool with three fixed-size buckets.
 *
 * Each bucket is described by:
 *   - buffer size
 *   - number of buffers
 *
 * Example:
 *
 *   buffer_pool_init(&pool,
 *                    256,   1024,
 *                    4096,  256,
 *                    65536, 64);
 */
int buffer_pool_init(
    BufferPool *pool,
    size_t size0, size_t capacity0,
    size_t size1, size_t capacity1,
    size_t size2, size_t capacity2
);


/*
 * Acquire a buffer capable of holding at least `size` bytes.
 *
 * Returns NULL if no suitable buffer is available.
 *
 * The smallest bucket capable of satisfying the requested size
 * is selected.
 */
uint8_t *buffer_pool_pop(
    BufferPool *pool,
    size_t* size
);


/*
 * Return a buffer to its bucket.
 *
 * `size` must be the size of the bucket from which the buffer
 * was obtained.
 *
 * Returns 0 on success and -1 if no matching bucket exists.
 */
int buffer_pool_push(
    BufferPool *pool,
    void *buffer,
    size_t size
);


/*
 * Release all memory owned by the pool.
 */
void buffer_pool_destroy(
    BufferPool *pool
);
