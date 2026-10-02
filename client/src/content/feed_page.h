/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "content/post.h"

#define FEED_PAGE_OK 0
#define FEED_PAGE_ERR -1
#define FEED_PAGE_FULL 1
#define FEED_PAGE_HEADER_SIZE 16
#define FEED_PAGE_MAX_SIZE (64 * 1024)
#define FEED_PAGE_MAX_POSTS ((FEED_PAGE_MAX_SIZE - FEED_PAGE_HEADER_SIZE) / POST_SIZE)

typedef struct FeedPage {
    // PostView is a byte pointer; entries are packed consecutively at POST_SIZE strides.
    PostView posts;
    size_t size;
    size_t cap;
    uint64_t page_number;
} FeedPage;

// Initialize fresh storage; destroy a live page before reinitializing it.
int feed_page_init(FeedPage* page, uint64_t page_number);
// Wipe the allocation, including package keys, before releasing it.
void feed_page_destroy(FeedPage* page);

int feed_page_count_posts(const uint8_t* bytes, size_t size, size_t* count);
// Validate and copy the entire batch; FULL and ERR leave the page unchanged.
int feed_page_append_posts(FeedPage* page, const uint8_t* bytes, size_t size);

static inline int feed_page_get_post(const FeedPage* page, size_t index, PostView* post) {
    if (!page || !post || !page->posts || page->size > page->cap ||
        page->cap > FEED_PAGE_MAX_POSTS || index >= page->size) {
        return FEED_PAGE_ERR;
    }

    *post = page->posts + index * POST_SIZE;
    return FEED_PAGE_OK;
}
