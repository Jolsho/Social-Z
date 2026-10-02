/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "codec/feed_page.h"
#include <stdlib.h>

static uint64_t read_uint(const uint8_t* bytes, size_t size) {
    uint64_t value = 0;

    for (size_t i = 0; i < size; i++) {
        value = (value << 8) | bytes[i];
    }

    return value;
}

static void write_uint(uint8_t* bytes, uint64_t value, size_t size) {
    while (size > 0) {
        bytes[--size] = (uint8_t)value;
        value >>= 8;
    }
}

int marshal_feed_page(
    uint8_t* out,
    size_t capacity,
    size_t* size,
    const FeedPage* page
) {
    if (!out || !size || !page ||
        page->size > page->cap || page->cap > FEED_PAGE_MAX_POSTS) {
        return FEED_PAGE_ERR;
    }

    size_t count;
    if (feed_page_count_posts(page->posts, page->size * POST_SIZE, &count) != FEED_PAGE_OK ||
        count != page->size) {
        return FEED_PAGE_ERR;
    }

    size_t total = FEED_PAGE_HEADER_SIZE + page->size * POST_SIZE;
    if (capacity < total) {
        return FEED_PAGE_ERR;
    }

    // Big-endian u16 fields: version 1, then record kind 3 (feed page).
    write_uint(out, 1, 2);
    write_uint(out + 2, 3, 2);
    write_uint(out + 4, page->page_number, 8);
    write_uint(out + 12, count, 4);

    for (size_t offset = 0; offset < page->size * POST_SIZE;) {
        const uint8_t* post = page->posts + offset;
        size_t post_size = POST_SIZE;
        uint8_t* encoded = out + FEED_PAGE_HEADER_SIZE + offset;

        memcpy(encoded, post, post_size);

        // Preserve the timestamp's 64 bits while changing its byte order for storage.
        uint64_t timestamp;
        memcpy(&timestamp, post + POST_CREATED_AT_OFFSET, sizeof(timestamp));
        write_uint(encoded + POST_CREATED_AT_OFFSET, timestamp, sizeof(timestamp));

        uint32_t blobs;
        memcpy(&blobs, post + POST_BLOB_COUNT_OFFSET, sizeof(blobs));
        write_uint(encoded + POST_BLOB_COUNT_OFFSET, blobs, sizeof(blobs));

        offset += post_size;
    }

    *size = total;
    return FEED_PAGE_OK;
}

int parse_feed_page(
    FeedPage* out,
    const uint8_t* bytes,
    size_t size
) {
    if (!out || !bytes || size < FEED_PAGE_HEADER_SIZE || size > FEED_PAGE_MAX_SIZE) {
        return FEED_PAGE_ERR;
    }

    if (read_uint(bytes, 2) != 1 || read_uint(bytes + 2, 2) != 3) {
        return FEED_PAGE_ERR;
    }

    size_t payload_size = size - FEED_PAGE_HEADER_SIZE;
    const uint8_t* payload = bytes + FEED_PAGE_HEADER_SIZE;
    size_t count;

    // Validate every post before allocating or replacing the caller's page.
    if (feed_page_count_posts(payload, payload_size, &count) != FEED_PAGE_OK ||
        count != read_uint(bytes + 12, 4)) {
        return FEED_PAGE_ERR;
    }

    FeedPage page = {0};
    page.page_number = read_uint(bytes + 4, 8);
    page.cap = count ? count : 1;
    page.posts = malloc(page.cap * POST_SIZE);
    if (!page.posts) {
        return FEED_PAGE_ERR;
    }

    size_t offset = 0;
    for (size_t i = 0; i < count; i++) {
        const uint8_t* post = payload + offset;
        size_t post_size = POST_SIZE;

        memcpy(page.posts + offset, post, post_size);

        uint64_t timestamp = read_uint(post + POST_CREATED_AT_OFFSET, 8);
        memcpy(page.posts + offset + POST_CREATED_AT_OFFSET, &timestamp, sizeof(timestamp));

        uint32_t blobs = (uint32_t)read_uint(post + POST_BLOB_COUNT_OFFSET, sizeof(blobs));
        memcpy(page.posts + offset + POST_BLOB_COUNT_OFFSET, &blobs, sizeof(blobs));

        offset += post_size;
    }

    page.size = count;

    feed_page_destroy(out);
    *out = page;

    return FEED_PAGE_OK;
}
