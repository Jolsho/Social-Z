/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#undef realloc
#include "content/feed.h"
#include <assert.h>
#include <stdlib.h>

static bool fail_resize;

void* feed_test_realloc(void* bytes, size_t size) {
    return fail_resize ? NULL : realloc(bytes, size);
}

int main(void) {
    Feed feed = {.current_page_number = 7};
    FeedPage staging = {0};
    assert(!feed_find_page(&feed, 7));
    assert(feed_page_init(&staging, 7) == FEED_PAGE_OK);
    uint8_t post[POST_SIZE] = {0};
    uint32_t count = 1;
    memcpy(post + POST_BLOB_COUNT_OFFSET, &count, sizeof(count));
    assert(feed_page_append_posts(&staging, post, sizeof(post)) == FEED_PAGE_OK);
    FeedPage original = staging;

    fail_resize = true;
    assert(feed_store_page(&feed, &staging) == FEED_PAGE_ERR);
    assert(!feed.pages && !feed.size && !feed.cap);
    assert(memcmp(&staging, &original, sizeof(staging)) == 0);
    fail_resize = false;
    assert(feed_store_page(&feed, &staging) == FEED_PAGE_OK);
    assert(!staging.posts && !staging.size && !staging.cap);
    assert(feed_find_page(&feed, 7)->posts == original.posts);

    // Out-of-order pages can grow the vector without changing existing post storage or navigation.
    const uint64_t numbers[] = {12, 3, 8, 2};
    for (size_t i = 0; i < sizeof(numbers) / sizeof(numbers[0]); i++) {
        assert(feed_page_init(&staging, numbers[i]) == FEED_PAGE_OK);
        if (i == 3) {
            Feed before = feed;
            FeedPage pending = staging;
            fail_resize = true;
            assert(feed_store_page(&feed, &staging) == FEED_PAGE_ERR);
            assert(memcmp(&feed, &before, sizeof(feed)) == 0);
            assert(memcmp(&staging, &pending, sizeof(staging)) == 0);
            fail_resize = false;
        }
        assert(feed_store_page(&feed, &staging) == FEED_PAGE_OK);
    }
    assert(feed.size == 5 && feed.current_page_number == 7);
    assert(feed_find_page(&feed, 7)->posts == original.posts);
    assert(feed_find_page(&feed, 3)->page_number == 3);
    assert(!feed_find_page(&feed, 99));

    // Refreshing an existing page moves the new posts without growing the vector.
    assert(feed_page_init(&staging, 7) == FEED_PAGE_OK);
    post[0] = 0xaa;
    assert(feed_page_append_posts(&staging, post, sizeof(post)) == FEED_PAGE_OK);
    uint8_t* replacement = staging.posts;
    assert(feed_store_page(&feed, &staging) == FEED_PAGE_OK);
    assert(feed.size == 5 && !staging.posts);
    assert(feed_find_page(&feed, 7)->posts == replacement);
    assert(memcmp(replacement, post, sizeof(post)) == 0);
    feed_page_destroy(&staging);
    feed_destroy(&feed);
    assert(!feed.pages && !feed.size && !feed.cap);
    return 0;
}
