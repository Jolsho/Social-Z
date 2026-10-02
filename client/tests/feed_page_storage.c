/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "content/feed_page.h"
#include <assert.h>

static size_t write_post(uint8_t* bytes, uint32_t blobs, uint8_t value)
{
    size_t size = POST_SIZE;
    memset(bytes, value, size);
    memcpy(bytes + POST_BLOB_COUNT_OFFSET, &blobs, sizeof(blobs));
    return size;
}

static void release_posts(FeedPage* posts)
{
    feed_page_destroy(posts);
}

static void test_append_and_growth(void)
{
    FeedPage posts = {0};
    assert(feed_page_init(&posts, 0) == FEED_PAGE_OK);
    uint8_t first[POST_SIZE];
    write_post(first, 1, 17);
    assert(feed_page_append_posts(&posts, first, sizeof(first)) == FEED_PAGE_OK);
    assert(posts.size == 1);
    assert(memcmp(posts.posts, first, sizeof(first)) == 0);

    uint8_t batch[140 * POST_SIZE];
    size_t batch_size = 0;
    for (size_t i = 0; i < 140; i++)
        batch_size += write_post(batch + batch_size, (uint8_t)(i % 5 + 1), (uint8_t)i);
    assert(feed_page_append_posts(&posts, batch, batch_size) == FEED_PAGE_OK);
    assert(posts.size == 141);
    assert(posts.cap >= posts.size && posts.cap <= FEED_PAGE_MAX_POSTS);
    assert(memcmp(posts.posts, first, sizeof(first)) == 0);
    assert(memcmp(posts.posts + sizeof(first), batch, batch_size) == 0);

    size_t offset = sizeof(first);
    for (size_t i = 0; i < 140; i++) {
        uint8_t* view = NULL;
        assert(feed_page_get_post(&posts, (uint32_t)(i + 1), &view) == FEED_PAGE_OK);
        assert(view == posts.posts + (i + 1) * POST_SIZE);
        size_t size = POST_SIZE;
        assert(memcmp(view, batch + offset - sizeof(first), size) == 0);
        offset += size;
    }
    size_t previous_size = posts.size * POST_SIZE;
    assert(feed_page_append_posts(&posts, posts.posts, previous_size) == FEED_PAGE_OK);
    assert(posts.size == 282);
    assert(memcmp(posts.posts, posts.posts + previous_size, previous_size) == 0);
    release_posts(&posts);
}

static void test_rejects_malformed_batch(void)
{
    FeedPage posts = {0};
    assert(feed_page_init(&posts, 0) == FEED_PAGE_OK);
    uint8_t valid[POST_SIZE];
    write_post(valid, 1, 27);
    assert(feed_page_append_posts(&posts, valid, sizeof(valid)) == FEED_PAGE_OK);

    uint8_t batch[2 * POST_SIZE];
    memcpy(batch, valid, sizeof(valid));
    memcpy(batch + sizeof(valid), valid, sizeof(valid));
    memset(batch + sizeof(valid) + POST_BLOB_COUNT_OFFSET, 0, sizeof(uint32_t));
    assert(feed_page_append_posts(&posts, batch, sizeof(batch)) == FEED_PAGE_ERR);
    assert(feed_page_append_posts(&posts, valid, sizeof(valid) - 1) == FEED_PAGE_ERR);
    memset(batch + POST_BLOB_COUNT_OFFSET, 0, sizeof(uint32_t));
    assert(feed_page_append_posts(&posts, batch, sizeof(valid)) == FEED_PAGE_ERR);
    assert(feed_page_append_posts(&posts, valid, 1) == FEED_PAGE_ERR);
    assert(feed_page_append_posts(&posts, valid, (uint64_t)UINT32_MAX + 1) == FEED_PAGE_ERR);
    assert(posts.size == 1);
    assert(memcmp(posts.posts, valid, sizeof(valid)) == 0);
    release_posts(&posts);
}

static void test_empty_and_invalid_access(void)
{
    FeedPage posts = {0};
    uint8_t* view = NULL;
    assert(feed_page_init(NULL, 0) == FEED_PAGE_ERR);
    assert(feed_page_init(&posts, 0) == FEED_PAGE_OK);
    assert(feed_page_append_posts(&posts, NULL, 0) == FEED_PAGE_OK);
    assert(feed_page_get_post(&posts, 0, &view) == FEED_PAGE_ERR);
    uint8_t post[POST_SIZE];
    write_post(post, 1, 9);
    assert(feed_page_append_posts(&posts, post, sizeof(post)) == FEED_PAGE_OK);
    assert(feed_page_get_post(&posts, 1, &view) == FEED_PAGE_ERR);
    assert(feed_page_get_post(&posts, 0, NULL) == FEED_PAGE_ERR);
    release_posts(&posts);
}

int main(void)
{
    test_append_and_growth();
    test_rejects_malformed_batch();
    test_empty_and_invalid_access();
    return 0;
}
