/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "data/feed.h"

int feed_init(Feed* f, size_t min_item_len) {
    if (!f) return FEED_ERR;
    memset(f, 0, sizeof(*f));
    if (!min_item_len || min_item_len > UINT32_MAX / 100)
        return FEED_ERR;

    f->index = calloc(100, sizeof(ItemIndex));
    f->data = calloc(100, min_item_len);
    if (!f->index || !f->data) {
        free(f->index);
        free(f->data);
        memset(f, 0, sizeof(*f));
        return FEED_ERR;
    }
    f->cap = 100;
    f->data_cap = 100 * min_item_len;
    return FEED_OK;
}

int feed_append(
    Feed* f, 
    uint8_t* b, uint64_t len, 
    size_t min_item_len,
    ValidItem valid, GetItemLength get_item_len
) {
    if (!f || !f->index || !f->data || !min_item_len || !valid || !get_item_len ||
        f->size > f->cap || f->data_size > f->data_cap ||
        f->data_size > UINT32_MAX || len > UINT32_MAX - f->data_size ||
        (len && !b)) return FEED_ERR;
    if (!len) return FEED_OK;

    // Validate the entire batch before changing the feed.
    size_t count = 0;
    for (size_t offset = 0; offset < len; count++) {
        size_t remaining = (size_t)len - offset;
        if (remaining < min_item_len || !valid(b + offset)) return FEED_ERR;
        size_t size = get_item_len(b + offset);
        if (size < min_item_len || size > remaining) return FEED_ERR;
        offset += size;
    }
    if (count > SIZE_MAX / sizeof(ItemIndex) - f->size) return FEED_ERR;

    size_t data_size = f->data_size + (size_t)len;
    size_t size = f->size + count;
    size_t data_cap = f->data_cap;
    size_t cap = f->cap;
    uint8_t* data = f->data;
    ItemIndex* index = f->index;
    if (data_size > data_cap) {
        data_cap = data_cap <= UINT32_MAX / 2 ? data_cap * 2 : UINT32_MAX;
        if (data_cap < data_size) data_cap = data_size;
        data = malloc(data_cap);
        if (!data) return FEED_ERR;
        memcpy(data, f->data, f->data_size);
    }
    if (size > cap) {
        cap = cap <= SIZE_MAX / sizeof(ItemIndex) / 2 ? cap * 2 : size;
        if (cap < size) cap = size;
        index = malloc(cap * sizeof(ItemIndex));
        if (!index) {
            if (data != f->data) free(data);
            return FEED_ERR;
        }
        memcpy(index, f->index, f->size * sizeof(ItemIndex));
    }

    // Copy before freeing old storage, including when b is a view into it.
    memmove(data + f->data_size, b, (size_t)len);
    if (data != f->data) free(f->data);
    if (index != f->index) free(f->index);
    f->data = data;
    f->data_cap = data_cap;
    f->index = index;
    f->cap = cap;

    size_t offset = f->data_size;
    while (f->size < size) {
        size_t item_size = get_item_len(f->data + offset);
        f->index[f->size++] = (ItemIndex){(uint32_t)offset, (uint32_t)item_size};
        offset += item_size;
    }
    f->data_size = data_size;

    return FEED_OK;
}
