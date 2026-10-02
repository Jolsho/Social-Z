/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "content/feed.h"
#include <stdlib.h>

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
