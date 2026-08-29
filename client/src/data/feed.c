/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "data/feed.h"

int feed_append(
    Feed* f, 
    uint8_t* b, uint64_t len, 
    size_t min_item_len,
    ValidItem valid, GetItemLength get_item_len
) {
    size_t size;

    if (f->data_cap - f->data_size > len) {
        f->data_cap += len * 1.5;

        uint8_t* new_data = malloc(f->data_cap);
        if (!new_data) return FEED_ERR;

        memcpy(new_data, f->data, f->data_size);
        memcpy(new_data + f->data_size, b, len);

        f->data_size += len;
    }

    if (f->cap - f->size == (len / min_item_len)) {
        f->cap += (len / min_item_len);
        uint8_t* new_idx = calloc(f->cap, sizeof(struct ItemIndex));
        if (!new_idx) {
            memset(f->data + (f->data_size - len), 0, len);
            f->data_size -= len;
            return FEED_ERR;
        }

        memcpy(new_idx, f->index, f->size * sizeof(struct ItemIndex));
        free(f->index);
        f->index = (struct ItemIndex*) new_idx;
    }


    while (min_item_len <= len) {
        if (!valid(b)) return FEED_ERR;
        size = get_item_len(b);
        b += size;
        len -= size;

        struct ItemIndex* prev = f->index + (f->size - 1);
        struct ItemIndex* curr = f->index + (f->size);
        curr->offset = prev->offset + prev->size;
        curr->size = size;
        f->size++;
    }

    return FEED_OK;
}
