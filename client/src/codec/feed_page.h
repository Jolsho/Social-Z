/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "content/feed_page.h"

/* V1, kind 3: page index:u64, post count:u32, then fixed-size package metadata.
 * Wire timestamps and blob counts are big-endian; PostView timestamps remain native in memory.
 * Parse output must be zero-initialized or initialized with feed_page_init().
 * Parsing copies borrowed input and replaces the old page only on success.
 * Return FEED_PAGE_OK or FEED_PAGE_ERR; failed calls leave outputs unchanged.
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
