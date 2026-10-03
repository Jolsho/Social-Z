/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "content/feed.h"
#include <stdlib.h>

FeedPage* feed_find_page(Feed* feed, uint64_t page_number) {
    // Pages may arrive out of order, so their vector positions are not page numbers.
    for (size_t i = 0; i < feed->size; i++) {
        if (feed->pages[i].page_number == page_number) {
            return &feed->pages[i];
        }
    }
    return NULL;
}

int feed_store_page(Feed* feed, FeedPage* page) {
    FeedPage* loaded = feed_find_page(feed, page->page_number);
    if (loaded) {
        // A refreshed page replaces the previous owned posts, including their package keys.
        feed_page_destroy(loaded);
    } else {
        if (feed->size == feed->cap) {
            if (feed->cap > SIZE_MAX / sizeof(FeedPage) / 2) {
                return FEED_PAGE_ERR;
            }
            size_t capacity = feed->cap ? feed->cap * 2 : 4;
            FeedPage* pages = realloc(feed->pages, capacity * sizeof(*pages));
            if (!pages) {
                return FEED_PAGE_ERR;
            }
            feed->pages = pages;
            feed->cap = capacity;
        }
        loaded = &feed->pages[feed->size++];
    }

    // Transfer ownership only after storage is available; cleanup can now discard empty staging.
    *loaded = *page;
    *page = (FeedPage){0};
    return FEED_PAGE_OK;
}

void feed_destroy(Feed* feed) {
    if (!feed) {
        return;
    }

    for (size_t i = 0; i < feed->size; i++) {
        feed_page_destroy(&feed->pages[i]);
    }
    free(feed->pages);
    memset(feed, 0, sizeof(*feed));
}
