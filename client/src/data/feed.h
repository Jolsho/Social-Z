/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "data/post.h"
#include <stdlib.h>

#define FEED_OK     0
#define FEED_ERR    -1

typedef struct {
    uint32_t offset;
    uint32_t size;
} ItemIndex;

typedef struct {

    // offsets and lengths into *data.
    ItemIndex*  index;
    
    // Data entries.
    uint8_t*    data;
    size_t      data_size;
    size_t      data_cap;

    size_t      size;
    size_t      cap;

    size_t      recip_count;
} Feed;


int feed_init(Feed* f, size_t min_item_len);
// Wipe packed data, including package keys, before releasing its storage.
void feed_destroy(Feed* f);

static inline int feed_get_item(Feed* f, uint32_t idx, uint8_t** item_view) {
    if (!f || !item_view || !f->index || !f->data || idx >= f->size)
        return FEED_ERR;
    *item_view = f->data + f->index[idx].offset;
    return FEED_OK;
}

static inline size_t feed_get_size(const Feed* f) { return f->size; }

typedef bool (*ValidItem)(uint8_t* b);
typedef size_t (*GetItemLength)(uint8_t* b);

int feed_append(
    Feed* f, 
    uint8_t* b, uint64_t len, 
    size_t min_item_len,
    ValidItem valid, GetItemLength get_item_len
);

static inline int feed_append_posts(Feed* f, uint8_t* b, uint64_t len) {
    return feed_append(f, b, len, POST_SIZE, post_is_valid, post_get_size);
}
