/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "data/feed.h"

#define FEED_PAGE_HEADER_SIZE 16
#define FEED_PAGE_MAX_SIZE (64 * 1024)
#define FEED_PAGE_FULL 1

typedef struct FeedPage {
    uint64_t index;
    Feed posts;
} FeedPage;

// Initialize fresh storage; destroy a live page before reinitializing it.
int feed_page_init(FeedPage* page, uint64_t index);
void feed_page_destroy(FeedPage* page);

// FULL leaves the page unchanged; publication decides when to start the next page.
int feed_page_append_posts(FeedPage* page, uint8_t* bytes, size_t size);

/* V1, kind 3: page index:u64, post count:u32, then fixed-size package metadata.
 * Wire timestamps and blob counts are big-endian; PostView timestamps remain native in memory.
 * The offset/size index is rebuilt locally and is never serialized.
 * Parse output must be zero-initialized or initialized with feed_page_init().
 * Parsing copies borrowed input and replaces the old page only on success.
 * Return FEED_OK or FEED_ERR; failed calls leave outputs unchanged.
 */
int marshal_feed_page(
    uint8_t* out,
    size_t capacity,
    size_t* size,
    const FeedPage* page
);

int parse_feed_page(
    FeedPage* out,
    const uint8_t* bytes,
    size_t size
);
