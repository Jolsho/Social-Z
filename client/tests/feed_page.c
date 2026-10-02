/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#undef malloc
#undef calloc
#include "codec/feed.h"
#include <assert.h>

static size_t allocation_count, fail_at;

void* feed_page_test_malloc(size_t size) {
    if (++allocation_count == fail_at) {
        return NULL;
    }

    return malloc(size);
}

void* feed_page_test_calloc(size_t count, size_t size) {
    if (++allocation_count == fail_at) {
        return NULL;
    }

    return calloc(count, size);
}

static size_t write_post(uint8_t* bytes, uint8_t hashes, int64_t timestamp) {
    size_t size = POST_HASH_OFFSET + hashes * HASH_SIZE;

    memset(bytes, 0xaa, KEY_SIZE);
    memcpy(bytes + POST_CREATED_AT_OFFSET, &timestamp, sizeof(timestamp));
    bytes[POST_HASH_COUNT_OFFSET] = hashes;
    memset(bytes + POST_HASH_OFFSET, 0xcc, hashes * HASH_SIZE);

    return size;
}

static void variable_posts_and_wire(void) {
    FeedPage page = {0}, parsed = {0};
    assert(feed_page_init(&page, UINT64_C(0x1112131415161718)) == FEED_OK);

    uint8_t posts[2 * (POST_HASH_OFFSET + 5 * HASH_SIZE)];
    size_t first_size = write_post(posts, 1, INT64_C(0x0102030405060708));
    size_t second_size = write_post(posts + first_size, 5, INT64_MIN);
    size_t payload_size = first_size + second_size;

    assert(feed_page_append_posts(&page, posts, payload_size) == FEED_OK);
    assert(page.posts.size == 2);

    uint8_t bytes[FEED_PAGE_MAX_SIZE];
    size_t size;
    assert(marshal_feed_page(bytes, sizeof(bytes), &size, &page) == FEED_OK);
    assert(size == FEED_PAGE_HEADER_SIZE + payload_size);

    const uint8_t header[] = {
        0, 1, 0, 3,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0, 0, 0, 2,
    };
    const uint8_t first_time[] = {1, 2, 3, 4, 5, 6, 7, 8};
    const uint8_t second_time[] = {0x80, 0, 0, 0, 0, 0, 0, 0};

    assert(memcmp(bytes, header, sizeof(header)) == 0);
    assert(memcmp(bytes + 16, posts, KEY_SIZE) == 0);
    assert(memcmp(bytes + 48, first_time, sizeof(first_time)) == 0);
    assert(bytes[56] == 1);
    assert(memcmp(bytes + 57, posts + POST_HASH_OFFSET, HASH_SIZE) == 0);
    assert(memcmp(bytes + 16 + first_size, posts + first_size, KEY_SIZE) == 0);
    assert(memcmp(bytes + 48 + first_size, second_time, sizeof(second_time)) == 0);
    assert(bytes[56 + first_size] == 5);
    assert(memcmp(bytes + 57 + first_size, posts + first_size + POST_HASH_OFFSET, 5 * HASH_SIZE) == 0);

    assert(parse_feed_page(&parsed, bytes, size) == FEED_OK);
    memset(bytes, 0, size);

    assert(parsed.index == page.index && parsed.posts.size == 2);
    assert(parsed.posts.data_size == payload_size);
    assert(memcmp(parsed.posts.data, posts, payload_size) == 0);
    assert(parsed.posts.index[0].offset == 0 && parsed.posts.index[0].size == first_size);
    assert(parsed.posts.index[1].offset == first_size && parsed.posts.index[1].size == second_size);

    uint8_t* view;
    assert(feed_get_item(&parsed.posts, 1, &view) == FEED_OK);
    assert(post_get_created_at(view) == INT64_MIN);

    // Replacing a page may consume a borrowed view into its own old allocation.
    assert(marshal_feed_page(bytes, sizeof(bytes), &size, &parsed) == FEED_OK);
    assert(page.posts.data_cap >= size);
    memcpy(page.posts.data, bytes, size);
    assert(parse_feed_page(&page, page.posts.data, size) == FEED_OK);
    assert(memcmp(page.posts.data, posts, payload_size) == 0);

    feed_page_destroy(&parsed);
    feed_page_destroy(&page);
}

static void rejected(FeedPage* page, const uint8_t* bytes, size_t size) {
    FeedPage before = *page;
    uint8_t post[MINIMUM_POST_SIZE];
    ItemIndex item = page->posts.index[0];
    memcpy(post, page->posts.data, sizeof(post));

    assert(parse_feed_page(page, bytes, size) == FEED_ERR);
    assert(page->index == before.index);
    assert(page->posts.index == before.posts.index && page->posts.data == before.posts.data);
    assert(page->posts.size == before.posts.size && page->posts.data_size == before.posts.data_size);
    assert(memcmp(page->posts.data, post, sizeof(post)) == 0);
    assert(memcmp(&page->posts.index[0], &item, sizeof(item)) == 0);
}

static void malformed_and_allocation_failure(void) {
    FeedPage page = {0};
    assert(feed_page_init(&page, 7) == FEED_OK);

    uint8_t post[MINIMUM_POST_SIZE];
    write_post(post, 1, 23);
    assert(feed_page_append_posts(&page, post, sizeof(post)) == FEED_OK);

    uint8_t bytes[FEED_PAGE_HEADER_SIZE + sizeof(post) + 1];
    size_t size;
    assert(marshal_feed_page(bytes, sizeof(bytes), &size, &page) == FEED_OK);

    for (size_t n = 0; n < size; n++) {
        rejected(&page, bytes, n);
    }

    bytes[size] = 0;
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

    bytes[16 + POST_HASH_COUNT_OFFSET] = 0;
    rejected(&page, bytes, size);
    bytes[16 + POST_HASH_COUNT_OFFSET] = 6;
    rejected(&page, bytes, size);
    bytes[16 + POST_HASH_COUNT_OFFSET] = 5;
    rejected(&page, bytes, size);
    bytes[16 + POST_HASH_COUNT_OFFSET] = 1;

    for (size_t failure = 1; failure <= 2; failure++) {
        allocation_count = 0;
        fail_at = failure;
        rejected(&page, bytes, size);
        fail_at = 0;
    }

    memset(bytes, 0xa5, sizeof(bytes));
    size = 999;
    assert(marshal_feed_page(bytes, FEED_PAGE_HEADER_SIZE + sizeof(post) - 1, &size, &page) == FEED_ERR);
    assert(size == 999);
    for (size_t i = 0; i < sizeof(bytes); i++) {
        assert(bytes[i] == 0xa5);
    }

    page.posts.size++;
    assert(marshal_feed_page(bytes, sizeof(bytes), &size, &page) == FEED_ERR);
    page.posts.size--;

    assert(feed_page_append_posts(&page, post, sizeof(post) - 1) == FEED_ERR);
    assert(page.posts.size == 1);
    feed_page_destroy(&page);
}

static void byte_limit_and_empty_page(void) {
    FeedPage page = {0}, parsed = {0};
    assert(feed_page_init(&page, UINT64_MAX) == FEED_OK);

    uint8_t bytes[FEED_PAGE_MAX_SIZE];
    size_t size;
    assert(marshal_feed_page(bytes, sizeof(bytes), &size, &page) == FEED_OK);
    assert(size == FEED_PAGE_HEADER_SIZE);
    assert(parse_feed_page(&parsed, bytes, size) == FEED_OK);
    assert(parsed.index == UINT64_MAX && parsed.posts.size == 0);
    assert(feed_page_append_posts(&parsed, NULL, 0) == FEED_OK);

    // Mix 336 variable-size posts to fill the 65,520-byte payload exactly.
    uint8_t posts[FEED_PAGE_MAX_SIZE - FEED_PAGE_HEADER_SIZE];
    size_t payload_size = 0;
    for (size_t i = 0; i < 336; i++) {
        uint8_t hashes = i < 320 ? 5 : (i == 320 ? 2 : 1);
        payload_size += write_post(posts + payload_size, hashes, (int64_t)i);
    }

    assert(payload_size == sizeof(posts));
    assert(feed_page_append_posts(&page, posts, payload_size) == FEED_OK);
    assert(page.posts.size == 336);
    assert(marshal_feed_page(bytes, sizeof(bytes), &size, &page) == FEED_OK);
    assert(size == FEED_PAGE_MAX_SIZE);
    assert(parse_feed_page(&parsed, bytes, size) == FEED_OK);
    assert(parsed.posts.size == 336 && memcmp(parsed.posts.data, posts, payload_size) == 0);

    uint8_t next[MINIMUM_POST_SIZE];
    write_post(next, 1, 337);
    uint8_t* data = page.posts.data;
    ItemIndex* index = page.posts.index;

    assert(feed_page_append_posts(&page, next, sizeof(next)) == FEED_PAGE_FULL);
    assert(page.index == UINT64_MAX && page.posts.size == 336 && page.posts.data_size == payload_size);
    assert(page.posts.data == data && page.posts.index == index);
    assert(memcmp(page.posts.data, posts, payload_size) == 0);

    FeedPage new_page = {0};
    assert(feed_page_init(&new_page, 8) == FEED_OK);
    assert(feed_page_append_posts(&new_page, next, sizeof(next)) == FEED_OK);
    assert(new_page.posts.size == 1 && new_page.posts.data_size == sizeof(next));

    feed_page_destroy(&new_page);
    feed_page_destroy(&parsed);
    feed_page_destroy(&page);
    feed_page_destroy(&page);
}

int main(void) {
    variable_posts_and_wire();
    malformed_and_allocation_failure();
    byte_limit_and_empty_page();

    return 0;
}
