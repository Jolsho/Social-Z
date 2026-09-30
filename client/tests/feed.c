/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "data/feed.h"
#include <assert.h>

static size_t write_post(uint8_t* bytes, uint8_t hashes, uint8_t value)
{
    size_t size = POST_HASH_OFFSET + hashes * HASH_SIZE;
    memset(bytes, value, size);
    bytes[POST_HASH_COUNT_OFFSET] = hashes;
    return size;
}

static void release_feed(Feed* feed)
{
    free(feed->index);
    free(feed->data);
}

static void test_append_and_growth(void)
{
    Feed feed = {0};
    assert(feed_init(&feed, MINIMUM_POST_SIZE) == FEED_OK);
    uint8_t first[MINIMUM_POST_SIZE];
    write_post(first, 1, 17);
    assert(feed_append_posts(&feed, first, sizeof(first)) == FEED_OK);
    assert(feed.size == 1 && feed.data_size == sizeof(first));
    assert(feed.index[0].offset == 0);
    assert(memcmp(feed.data, first, sizeof(first)) == 0);

    uint8_t batch[140 * (POST_HASH_OFFSET + 5 * HASH_SIZE)];
    size_t batch_size = 0;
    for (size_t i = 0; i < 140; i++)
        batch_size += write_post(batch + batch_size, (uint8_t)(i % 5 + 1), (uint8_t)i);
    assert(feed_append_posts(&feed, batch, batch_size) == FEED_OK);
    assert(feed.size == 141 && feed.data_size == sizeof(first) + batch_size);
    assert(feed.cap >= feed.size && feed.data_cap >= feed.data_size);
    assert(memcmp(feed.data, first, sizeof(first)) == 0);
    assert(memcmp(feed.data + sizeof(first), batch, batch_size) == 0);

    size_t offset = sizeof(first);
    for (size_t i = 0; i < 140; i++) {
        uint8_t* view = NULL;
        assert(feed_get_item(&feed, (uint32_t)(i + 1), &view) == FEED_OK);
        size_t size = POST_HASH_OFFSET + (i % 5 + 1) * HASH_SIZE;
        assert(feed.index[i + 1].offset == offset);
        assert(feed.index[i + 1].size == size);
        assert(memcmp(view, batch + offset - sizeof(first), size) == 0);
        offset += size;
    }
    size_t previous_size = feed.data_size;
    assert(feed_append_posts(&feed, feed.data, previous_size) == FEED_OK);
    assert(feed.size == 282 && feed.data_size == 2 * previous_size);
    assert(memcmp(feed.data, feed.data + previous_size, previous_size) == 0);
    assert(feed.index[141].offset == previous_size);
    release_feed(&feed);
}

static void test_rejects_malformed_batch(void)
{
    Feed feed = {0};
    assert(feed_init(&feed, MINIMUM_POST_SIZE) == FEED_OK);
    uint8_t valid[MINIMUM_POST_SIZE];
    write_post(valid, 1, 27);
    assert(feed_append_posts(&feed, valid, sizeof(valid)) == FEED_OK);

    uint8_t batch[2 * MINIMUM_POST_SIZE];
    memcpy(batch, valid, sizeof(valid));
    memcpy(batch + sizeof(valid), valid, sizeof(valid));
    batch[sizeof(valid) + POST_HASH_COUNT_OFFSET] = 5;
    assert(feed_append_posts(&feed, batch, sizeof(batch)) == FEED_ERR);
    assert(feed_append_posts(&feed, valid, sizeof(valid) - 1) == FEED_ERR);
    batch[POST_HASH_COUNT_OFFSET] = 0;
    assert(feed_append_posts(&feed, batch, sizeof(valid)) == FEED_ERR);
    assert(feed_append_posts(&feed, valid, 1) == FEED_ERR);
    assert(feed_append_posts(&feed, valid, (uint64_t)UINT32_MAX + 1) == FEED_ERR);
    assert(feed.size == 1 && feed.data_size == sizeof(valid));
    assert(memcmp(feed.data, valid, sizeof(valid)) == 0);
    assert(feed.index[0].offset == 0 && feed.index[0].size == sizeof(valid));
    release_feed(&feed);
}

static void test_empty_and_invalid_access(void)
{
    Feed feed = {0};
    uint8_t* view = NULL;
    assert(feed_init(&feed, 0) == FEED_ERR);
    assert(feed_init(&feed, UINT32_MAX) == FEED_ERR);
    assert(feed_init(&feed, MINIMUM_POST_SIZE) == FEED_OK);
    assert(feed_append_posts(&feed, NULL, 0) == FEED_OK);
    assert(feed_get_item(&feed, 0, &view) == FEED_ERR);
    uint8_t post[MINIMUM_POST_SIZE];
    write_post(post, 1, 9);
    assert(feed_append_posts(&feed, post, sizeof(post)) == FEED_OK);
    assert(feed_get_item(&feed, 1, &view) == FEED_ERR);
    assert(feed_get_item(&feed, 0, NULL) == FEED_ERR);
    release_feed(&feed);
}

int main(void)
{
    test_append_and_growth();
    test_rejects_malformed_batch();
    test_empty_and_invalid_access();
    return 0;
}
