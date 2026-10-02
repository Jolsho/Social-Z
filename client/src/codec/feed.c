/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "codec/feed.h"

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

static int count_posts(const uint8_t* bytes, size_t size, size_t* count) {
    if (size && !bytes) {
        return FEED_ERR;
    }

    *count = 0;

    if (size % POST_SIZE != 0) {
        return FEED_ERR;
    }

    for (size_t offset = 0; offset < size; offset += POST_SIZE) {
        uint32_t blobs;
        memcpy(&blobs, bytes + offset + POST_BLOB_COUNT_OFFSET, sizeof(blobs));
        if (blobs == 0) {
            return FEED_ERR;
        }
        (*count)++;
    }

    return FEED_OK;
}

int feed_page_init(FeedPage* page, uint64_t index) {
    if (!page) {
        return FEED_ERR;
    }

    memset(page, 0, sizeof(*page));
    page->index = index;

    return feed_init(&page->posts, POST_SIZE);
}

void feed_page_destroy(FeedPage* page) {
    if (!page) {
        return;
    }

    feed_destroy(&page->posts);
    memset(page, 0, sizeof(*page));
}

int feed_page_append_posts(FeedPage* page, uint8_t* bytes, size_t size) {
    size_t count;
    size_t limit = FEED_PAGE_MAX_SIZE - FEED_PAGE_HEADER_SIZE;

    if (!page || page->posts.data_size > limit || size > limit) {
        return FEED_ERR;
    }

    if (count_posts(bytes, size, &count) != FEED_OK) {
        return FEED_ERR;
    }

    if (size > limit - page->posts.data_size) {
        return FEED_PAGE_FULL;
    }

    return feed_append_posts(&page->posts, bytes, size);
}

int marshal_feed_page(
    uint8_t* out,
    size_t capacity,
    size_t* size,
    const FeedPage* page
) {
    if (!out || !size || !page ||
        page->posts.data_size > FEED_PAGE_MAX_SIZE - FEED_PAGE_HEADER_SIZE) {
        return FEED_ERR;
    }

    size_t count;
    if (count_posts(page->posts.data, page->posts.data_size, &count) != FEED_OK ||
        count != page->posts.size) {
        return FEED_ERR;
    }

    size_t total = FEED_PAGE_HEADER_SIZE + page->posts.data_size;
    if (capacity < total) {
        return FEED_ERR;
    }

    // Big-endian u16 fields: version 1, then record kind 3 (feed page).
    write_uint(out, 1, 2);
    write_uint(out + 2, 3, 2);
    write_uint(out + 4, page->index, 8);
    write_uint(out + 12, count, 4);

    for (size_t offset = 0; offset < page->posts.data_size;) {
        const uint8_t* post = page->posts.data + offset;
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
    return FEED_OK;
}

int parse_feed_page(
    FeedPage* out,
    const uint8_t* bytes,
    size_t size
) {
    if (!out || !bytes || size < FEED_PAGE_HEADER_SIZE || size > FEED_PAGE_MAX_SIZE) {
        return FEED_ERR;
    }

    if (read_uint(bytes, 2) != 1 || read_uint(bytes + 2, 2) != 3) {
        return FEED_ERR;
    }

    size_t payload_size = size - FEED_PAGE_HEADER_SIZE;
    const uint8_t* payload = bytes + FEED_PAGE_HEADER_SIZE;
    size_t count;

    // Validate every post before allocating or replacing the caller's page.
    if (count_posts(payload, payload_size, &count) != FEED_OK ||
        count != read_uint(bytes + 12, 4)) {
        return FEED_ERR;
    }

    FeedPage page = {0};
    page.index = read_uint(bytes + 4, 8);
    page.posts.cap = count ? count : 1;
    page.posts.data_cap = payload_size ? payload_size : POST_SIZE;
    page.posts.index = calloc(page.posts.cap, sizeof(ItemIndex));
    page.posts.data = malloc(page.posts.data_cap);

    if (!page.posts.index || !page.posts.data) {
        feed_page_destroy(&page);
        return FEED_ERR;
    }

    size_t offset = 0;
    for (size_t i = 0; i < count; i++) {
        const uint8_t* post = payload + offset;
        size_t post_size = POST_SIZE;

        memcpy(page.posts.data + offset, post, post_size);

        uint64_t timestamp = read_uint(post + POST_CREATED_AT_OFFSET, 8);
        memcpy(page.posts.data + offset + POST_CREATED_AT_OFFSET, &timestamp, sizeof(timestamp));

        uint32_t blobs = (uint32_t)read_uint(post + POST_BLOB_COUNT_OFFSET, sizeof(blobs));
        memcpy(page.posts.data + offset + POST_BLOB_COUNT_OFFSET, &blobs, sizeof(blobs));

        page.posts.index[i] = (ItemIndex){(uint32_t)offset, (uint32_t)post_size};
        offset += post_size;
    }

    page.posts.size = count;
    page.posts.data_size = payload_size;

    feed_page_destroy(out);
    *out = page;

    return FEED_OK;
}
