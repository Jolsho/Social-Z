/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#undef malloc
#undef calloc
#undef free
#include "codec/feed.h"
#include <assert.h>
#include <sodium.h>

static size_t allocation_count, fail_at;
static uint8_t* watched_data;
static size_t watched_size;

void* feed_page_test_malloc(size_t size) {
    if (++allocation_count == fail_at) return NULL;
    void* memory = malloc(size);
    // When index growth will fail next, watch the temporary copy of secret metadata.
    if (fail_at == 2 && allocation_count == 1) {
        watched_data = memory;
        watched_size = size;
    }
    return memory;
}

void* feed_page_test_calloc(size_t count, size_t size) {
    return ++allocation_count == fail_at ? NULL : calloc(count, size);
}

void feed_page_test_free(void* memory) {
    if (memory && memory == watched_data) {
        assert(sodium_is_zero(memory, watched_size));
        watched_data = NULL;
    }
    free(memory);
}

static void watch(const FeedPage* page) {
    watched_data = page->posts.data;
    watched_size = page->posts.data_cap;
}

static void write_post(uint8_t* bytes, uint32_t blobs, int64_t timestamp) {
    memset(bytes, 0xaa, KEY_SIZE);
    memcpy(bytes + POST_CREATED_AT_OFFSET, &timestamp, sizeof(timestamp));
    memset(bytes + POST_PACKAGE_HASH_OFFSET, 0xcc, HASH_SIZE);
    memset(bytes + POST_PACKAGE_KEY_OFFSET, 0xdd, KEY_SIZE);
    memcpy(bytes + POST_BLOB_COUNT_OFFSET, &blobs, sizeof(blobs));
}

static void metadata_and_wire(void) {
    FeedPage page = {0}, parsed = {0};
    assert(POST_SIZE == 108);
    assert(feed_page_init(&page, UINT64_C(0x1112131415161718)) == FEED_OK);
    uint8_t posts[2 * POST_SIZE];
    write_post(posts, 2, INT64_C(0x0102030405060708));
    write_post(posts + POST_SIZE, UINT32_MAX, INT64_MIN);
    assert(feed_page_append_posts(&page, posts, sizeof(posts)) == FEED_OK);

    uint8_t bytes[FEED_PAGE_MAX_SIZE];
    size_t size;
    assert(marshal_feed_page(bytes, sizeof(bytes), &size, &page) == FEED_OK);
    assert(size == FEED_PAGE_HEADER_SIZE + sizeof(posts));
    const uint8_t header[] = {
        0, 1, 0, 3,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0, 0, 0, 2,
    };
    assert(memcmp(bytes, header, sizeof(header)) == 0);
    uint8_t expected[2 * POST_SIZE];
    memcpy(expected, posts, sizeof(posts));
    const uint8_t times[][8] = {{1, 2, 3, 4, 5, 6, 7, 8}, {0x80, 0, 0, 0, 0, 0, 0, 0}};
    const uint8_t counts[][4] = {{0, 0, 0, 2}, {0xff, 0xff, 0xff, 0xff}};
    for (size_t i = 0; i < 2; i++) {
        memcpy(expected + i * POST_SIZE + POST_CREATED_AT_OFFSET, times[i], 8);
        memcpy(expected + i * POST_SIZE + POST_BLOB_COUNT_OFFSET, counts[i], 4);
    }
    assert(memcmp(bytes + FEED_PAGE_HEADER_SIZE, expected, sizeof(expected)) == 0);
    assert(parse_feed_page(&parsed, bytes, size) == FEED_OK);
    memset(bytes, 0, size);
    assert(parsed.index == page.index && parsed.posts.size == 2);
    assert(memcmp(parsed.posts.data, posts, sizeof(posts)) == 0);

    uint8_t* view;
    assert(feed_get_item(&parsed.posts, 1, &view) == FEED_OK);
    assert(post_get_created_at(view) == INT64_MIN);
    assert(post_get_blob_count(view) == UINT32_MAX);
    Key origin, key;
    HashT hash;
    post_get_originator(view, &origin);
    post_get_package_hash(view, &hash);
    post_get_package_key(view, &key);
    assert(memcmp(origin.b, posts, KEY_SIZE) == 0);
    assert(memcmp(hash.b, posts + POST_PACKAGE_HASH_OFFSET, HASH_SIZE) == 0);
    assert(memcmp(key.b, posts + POST_PACKAGE_KEY_OFFSET, KEY_SIZE) == 0);
    assert(parsed.posts.index[1].offset == POST_SIZE && parsed.posts.index[1].size == POST_SIZE);

    // Parsing can replace its own old allocation after copying the borrowed input.
    assert(marshal_feed_page(bytes, sizeof(bytes), &size, &parsed) == FEED_OK);
    assert(page.posts.data_cap >= size);
    memcpy(page.posts.data, bytes, size);
    watch(&page);
    assert(parse_feed_page(&page, page.posts.data, size) == FEED_OK);
    assert(watched_data == NULL);
    assert(memcmp(page.posts.data, posts, sizeof(posts)) == 0);
    watch(&parsed);
    feed_page_destroy(&parsed);
    assert(watched_data == NULL);
    feed_page_destroy(&page);
}

static void rejected(FeedPage* page, const uint8_t* bytes, size_t size) {
    FeedPage before = *page;
    uint8_t post[POST_SIZE];
    ItemIndex item = page->posts.index[0];
    memcpy(post, page->posts.data, sizeof(post));
    assert(parse_feed_page(page, bytes, size) == FEED_ERR);
    assert(page->index == before.index && page->posts.index == before.posts.index);
    assert(page->posts.data == before.posts.data && page->posts.size == before.posts.size);
    assert(page->posts.data_size == before.posts.data_size);
    assert(memcmp(page->posts.data, post, sizeof(post)) == 0);
    assert(memcmp(&page->posts.index[0], &item, sizeof(item)) == 0);
}

static void malformed_and_allocation_failure(void) {
    FeedPage page = {0};
    assert(feed_page_init(&page, 7) == FEED_OK);
    uint8_t post[POST_SIZE];
    write_post(post, 1, 23);
    assert(feed_page_append_posts(&page, post, sizeof(post)) == FEED_OK);
    uint8_t bytes[FEED_PAGE_HEADER_SIZE + sizeof(post) + 1];
    size_t size;
    assert(marshal_feed_page(bytes, sizeof(bytes), &size, &page) == FEED_OK);
    for (size_t n = 0; n < size; n++) rejected(&page, bytes, n);
    rejected(&page, bytes, size + 1);
    rejected(&page, bytes, FEED_PAGE_MAX_SIZE + 1);
    rejected(&page, NULL, size);
    assert(parse_feed_page(NULL, bytes, size) == FEED_ERR);

    const size_t fields[] = {0, 1, 2, 3, 12, 15};
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        bytes[fields[i]] ^= 1;
        rejected(&page, bytes, size);
        bytes[fields[i]] ^= 1;
    }
    bytes[1] = 2; // Unsupported versions are still rejected.
    rejected(&page, bytes, size);
    FeedPage empty = {0};
    assert(parse_feed_page(&empty, bytes, FEED_PAGE_HEADER_SIZE) == FEED_ERR);
    bytes[1] = 1;
    memset(bytes + FEED_PAGE_HEADER_SIZE + POST_BLOB_COUNT_OFFSET, 0, 4);
    rejected(&page, bytes, size);
    bytes[FEED_PAGE_HEADER_SIZE + POST_BLOB_COUNT_OFFSET + 3] = 1;

    for (size_t failure = 1; failure <= 2; failure++) {
        allocation_count = 0;
        fail_at = failure;
        rejected(&page, bytes, size);
        fail_at = 0;
    }
    uint8_t out[sizeof(bytes)];
    memset(out, 0xa5, sizeof(out));
    size_t unchanged_size = 999;
    assert(marshal_feed_page(out, size - 1, &unchanged_size, &page) == FEED_ERR);
    assert(unchanged_size == 999);
    for (size_t i = 0; i < sizeof(out); i++) assert(out[i] == 0xa5);
    page.posts.size++;
    assert(marshal_feed_page(out, sizeof(out), &unchanged_size, &page) == FEED_ERR);
    page.posts.size--;
    assert(feed_page_append_posts(&page, post, sizeof(post) - 1) == FEED_ERR);
    assert(page.posts.size == 1);

    uint8_t batch[100 * POST_SIZE];
    for (size_t i = 0; i < 100; i++) write_post(batch + i * POST_SIZE, 1, (int64_t)i);
    FeedPage before = page;
    allocation_count = 0;
    fail_at = 2; // Data grows first; index allocation then fails.
    assert(feed_page_append_posts(&page, batch, sizeof(batch)) == FEED_ERR);
    fail_at = 0;
    assert(watched_data == NULL);
    assert(page.posts.data == before.posts.data && page.posts.index == before.posts.index);
    assert(page.posts.size == 1 && page.posts.data_size == POST_SIZE);
    assert(memcmp(page.posts.data, post, sizeof(post)) == 0);
    feed_page_destroy(&page);
}

static void page_capacity(void) {
    FeedPage page = {0}, parsed = {0};
    assert(feed_page_init(&page, UINT64_MAX) == FEED_OK);
    uint8_t bytes[FEED_PAGE_MAX_SIZE];
    size_t size;
    assert(marshal_feed_page(bytes, sizeof(bytes), &size, &page) == FEED_OK);
    assert(size == FEED_PAGE_HEADER_SIZE);
    assert(parse_feed_page(&parsed, bytes, size) == FEED_OK);
    assert(parsed.posts.size == 0 && parsed.index == UINT64_MAX);
    assert(feed_page_append_posts(&parsed, NULL, 0) == FEED_OK);

    enum { COUNT = (FEED_PAGE_MAX_SIZE - FEED_PAGE_HEADER_SIZE) / POST_SIZE };
    uint8_t posts[COUNT * POST_SIZE];
    for (size_t i = 0; i < COUNT; i++) write_post(posts + i * POST_SIZE, (uint32_t)i + 1, (int64_t)i);
    watch(&page); // Growth also wipes the old metadata allocation before releasing it.
    assert(feed_page_append_posts(&page, posts, sizeof(posts)) == FEED_OK);
    assert(watched_data == NULL);
    assert(page.posts.size == COUNT && COUNT == 606);
    assert(marshal_feed_page(bytes, sizeof(bytes), &size, &page) == FEED_OK);
    assert(size == FEED_PAGE_HEADER_SIZE + sizeof(posts));
    assert(parse_feed_page(&parsed, bytes, size) == FEED_OK);
    assert(memcmp(parsed.posts.data, posts, sizeof(posts)) == 0);
    uint8_t next[POST_SIZE];
    write_post(next, 8, 42);
    FeedPage before = page;
    assert(feed_page_append_posts(&page, next, sizeof(next)) == FEED_PAGE_FULL);
    assert(page.index == UINT64_MAX && page.posts.size == COUNT);
    assert(page.posts.data == before.posts.data && page.posts.index == before.posts.index);
    assert(memcmp(page.posts.data, posts, sizeof(posts)) == 0);

    FeedPage next_page = {0};
    assert(feed_page_init(&next_page, 8) == FEED_OK);
    assert(feed_page_append_posts(&next_page, next, sizeof(next)) == FEED_OK);
    assert(next_page.posts.size == 1);
    feed_page_destroy(&next_page);
    feed_page_destroy(&parsed);
    feed_page_destroy(&page);
    feed_page_destroy(&page);
}

int main(void) {
    metadata_and_wire();
    malformed_and_allocation_failure();
    page_capacity();
    return 0;
}
