/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "content/feed_page.h"
#include <stdlib.h>
#include <sodium.h>

int feed_page_init(FeedPage* page, uint64_t page_number) {
    if (!page) {
        return FEED_PAGE_ERR;
    }

    memset(page, 0, sizeof(*page));
    page->posts = calloc(100, POST_SIZE);
    if (!page->posts) {
        return FEED_PAGE_ERR;
    }

    page->cap = 100;
    page->page_number = page_number;
    return FEED_PAGE_OK;
}

void feed_page_destroy(FeedPage* page) {
    if (!page) {
        return;
    }

    if (page->posts) {
        sodium_memzero(page->posts, page->cap * POST_SIZE);
        free(page->posts);
    }
    memset(page, 0, sizeof(*page));
}

int feed_page_count_posts(const uint8_t* bytes, size_t size, size_t* count) {
    if (!count || (size && !bytes) || size % POST_SIZE != 0 ||
        size / POST_SIZE > FEED_PAGE_MAX_POSTS) {
        return FEED_PAGE_ERR;
    }

    for (size_t offset = 0; offset < size; offset += POST_SIZE) {
        uint32_t blobs;
        memcpy(&blobs, bytes + offset + POST_BLOB_COUNT_OFFSET, sizeof(blobs));
        if (blobs == 0) {
            return FEED_PAGE_ERR;
        }
    }

    *count = size / POST_SIZE;
    return FEED_PAGE_OK;
}

int feed_page_append_posts(FeedPage* page, const uint8_t* bytes, size_t size) {
    size_t count;
    if (!page || !page->posts || page->size > page->cap ||
        page->cap > FEED_PAGE_MAX_POSTS ||
        feed_page_count_posts(bytes, size, &count) != FEED_PAGE_OK) {
        return FEED_PAGE_ERR;
    }
    if (count > FEED_PAGE_MAX_POSTS - page->size) {
        return FEED_PAGE_FULL;
    }
    if (!count) {
        return FEED_PAGE_OK;
    }

    size_t total = page->size + count;
    size_t cap = page->cap;
    uint8_t* posts = page->posts;
    if (total > cap) {
        cap *= 2;
        if (cap < total) cap = total;
        if (cap > FEED_PAGE_MAX_POSTS) cap = FEED_PAGE_MAX_POSTS;
        posts = malloc(cap * POST_SIZE);
        if (!posts) {
            return FEED_PAGE_ERR;
        }
        memcpy(posts, page->posts, page->size * POST_SIZE);
    }

    // Copy before wiping the old allocation: bytes may be a view into that allocation.
    memmove(posts + page->size * POST_SIZE, bytes, size);
    if (posts != page->posts) {
        sodium_memzero(page->posts, page->cap * POST_SIZE);
        free(page->posts);
    }
    page->posts = posts;
    page->size = total;
    page->cap = cap;
    return FEED_PAGE_OK;
}
