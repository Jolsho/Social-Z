/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_common/vec.h"
#include "sz_common/codec.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t* PostView;

#define POST_ORIGIN_OFFSET       0
#define POST_CREATED_AT_OFFSET   (POST_ORIGIN_OFFSET + KEY_SIZE)
#define POST_CREATED_AT_SIZE     sizeof(int64_t)
#define POST_PACKAGE_HASH_OFFSET (POST_CREATED_AT_OFFSET + POST_CREATED_AT_SIZE)
#define POST_PACKAGE_KEY_OFFSET  (POST_PACKAGE_HASH_OFFSET + HASH_SIZE)
#define POST_BLOB_COUNT_OFFSET   (POST_PACKAGE_KEY_OFFSET + KEY_SIZE)
#define POST_SIZE                (POST_BLOB_COUNT_OFFSET + sizeof(uint32_t))

// Packed native metadata in memory; the feed codec handles integer byte order.
static inline void post_get_originator(PostView post, Key* key) {
    memcpy(key->b, post + POST_ORIGIN_OFFSET, KEY_SIZE);
}

static inline int64_t post_get_created_at(PostView post) {
    int64_t created_at;
    memcpy(&created_at, post + POST_CREATED_AT_OFFSET, sizeof(created_at));
    return created_at;
}

static inline void post_get_package_hash(PostView post, HashT* hash) {
    memcpy(hash->b, post + POST_PACKAGE_HASH_OFFSET, HASH_SIZE);
}

static inline void post_get_package_key(PostView post, Key* key) {
    memcpy(key->b, post + POST_PACKAGE_KEY_OFFSET, KEY_SIZE);
}

static inline uint32_t post_get_blob_count(PostView post) {
    uint32_t count;
    memcpy(&count, post + POST_BLOB_COUNT_OFFSET, sizeof(count));
    return count;
}

static inline size_t post_get_size(PostView post) {
    (void)post;
    return POST_SIZE;
}

static inline bool post_is_valid(PostView post) {
    return post && post_get_blob_count(post) > 0;
}

#ifdef __cplusplus
}
#endif
