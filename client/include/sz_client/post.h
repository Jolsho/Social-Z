
/*
 * Copyright (c) 2026 Jolsho
 *
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

#define POST_ORIGIN_OFFSET      0
#define POST_CREATED_AT_OFFSET  (POST_ORIGIN_OFFSET + KEY_SIZE)
#define POST_CREATED_AT_SIZE    sizeof(int64_t)
#define POST_HASH_COUNT_OFFSET  (POST_CREATED_AT_OFFSET + POST_CREATED_AT_SIZE)
#define POST_HASH_OFFSET        (POST_HASH_COUNT_OFFSET + 1)

// Must have atleast one associated data hash.
#define MINIMUM_POST_SIZE       (POST_HASH_OFFSET + HASH_SIZE)

static inline void post_get_originator(PostView p, Key* k) {
    memcpy(k->b, p + POST_ORIGIN_OFFSET, KEY_SIZE); 
}

static inline int64_t post_get_created_at(PostView p) { 
    int64_t created_at;
    memcpy(&created_at, p + POST_CREATED_AT_OFFSET, POST_CREATED_AT_SIZE); 
    return created_at;
}

static inline uint8_t post_get_hash_count(PostView p) { 
    return *(p + POST_HASH_COUNT_OFFSET);
}

static inline void post_get_hash(PostView p, HashT* h, size_t idx) { 
    memcpy(h->b, p + POST_HASH_OFFSET + (HASH_SIZE * idx), HASH_SIZE); 
}

static inline size_t post_get_size(PostView p) {
    return POST_HASH_OFFSET + (*(p + POST_HASH_COUNT_OFFSET) * HASH_SIZE);
}

static inline bool post_is_valid(PostView p) {
    uint8_t hash_count = post_get_hash_count(p);
    return hash_count <= 5 && hash_count > 0;
}



#ifdef __cplusplus
}
#endif
