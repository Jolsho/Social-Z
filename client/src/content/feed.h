/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "content/feed_page.h"

struct Client;

typedef struct Feed {
    // Owned loaded pages; page numbers need not match their positions in this vector.
    FeedPage* pages;
    size_t size;
    size_t cap;
    uint64_t current_page_number;
} Feed;

// A zero-initialized Feed is empty; destruction also releases every loaded page.
void feed_destroy(Feed* feed);

// TODO: Define navigation and pending-load results before implementing these functions.
// Missing pages may initiate a request or queue work through the client.
// Returned page views will be borrowed and may move when the vector grows.
FeedPage* feed_get_current_page(struct Client* client, Feed* feed);
int feed_next_page(struct Client* client, Feed* feed);
