/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz/client/iterator.h"
#include "sz/api/vec.h"
#include "sz/codec.h"
#include <sodium/crypto_aead_chacha20poly1305.h>
#include <sodium/crypto_pwhash.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

Iterator* iterator_new(int obj_enum) {
    Iterator* it = malloc(sizeof(Iterator));
    it->bufs = malloc(30 * sizeof(Card*));
    it->bufs_cap = 30;
    it->buff_idx = 0;
    it->item_idx = 0;

    it->sizes = malloc(30 * sizeof(size_t));
    it->size = 0;
    
    switch (obj_enum) { 
        case OBJ_CARD: {
            it->obj_size = sizeof(Card);
            break;
        }
        case OBJ_USER: {
            it->obj_size = 1;
            break;
        }
        default: {
            free(it->bufs);
            free(it);
            return NULL;
        }
    }
    return it;
}

size_t iterator_remaining(Iterator* it) { 
    size_t remaining;
    for (int i = it->buff_idx; i < it->bufs_size; i++) {
        if (i == it->buff_idx) {
            remaining += (it->sizes[i] - it->item_idx);
        } else {
            remaining += it->sizes[i];
        }
    }
    return remaining; 
}

void* iterator_seek(Iterator* it, size_t idx) {
    if (idx > it->size) return NULL;

    size_t end_range;
    for (int i = 0; i < it->bufs_size; i++) {
        end_range += it->sizes[i];
        if (end_range > idx) {
            it->buff_idx = i;
            break;
        }
    }
    it->item_idx = end_range - idx;
    return it->bufs[it->buff_idx]->b + (it->item_idx * it->obj_size);
}

void* iterator_next(Iterator* it) {
    if (it->item_idx == it->sizes[it->buff_idx]) {
        if (it->buff_idx == it->bufs_cap - 1) return NULL;
        it->buff_idx++;
        it->item_idx = 0;
    }
    return it->bufs[it->buff_idx]->b + (it->item_idx++ * it->obj_size);

}

void* iterator_prev(Iterator* it) {
    if (it->item_idx < 0) {
        if (it->buff_idx == 0) return NULL;
        it->buff_idx--;
        it->item_idx = it->sizes[it->buff_idx] - 1;
    }
    return it->bufs[it->buff_idx]->b + (it->item_idx-- * it->obj_size);
}

bool iterator_append(Iterator* it, uint8_t* buf, uint64_t len) {
    if (it->bufs_size == it->bufs_cap - 1) {
        Vec** bufs = malloc((it->bufs_cap += 10) * sizeof(uint8_t*));
        if (!bufs) return false;

        memcpy(bufs, it->bufs, it->bufs_size * sizeof(uint8_t*));
        free(it->bufs);
        it->bufs = bufs;
    }

    Vec* v = new_vec(len);
    if (!v) return false;

    size_t vidx = it->bufs_size++;
    it->bufs[vidx] = v;
    memcpy(v->b, buf, len);
    v->len = len;
    it->sizes[vidx] = len / it->obj_size;
    it->size += it->sizes[vidx];

    return true;
}
