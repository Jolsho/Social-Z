/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz/api/vec.h"
#include "sz/utils/buffers.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


BufferCaps* default_caps() {
    BufferCaps* caps = malloc(sizeof(BufferCaps));

    caps->xxs = 16;
    caps->xs  = 16;
    caps->s   = 16;
    caps->m   = 16;
    caps->l   = 16;
    caps->xl  = 16;
    caps->xxl = 16;
    caps->su  = 16;

    return caps;
}


static void buffer_store_init_bucket(
    BufferStore* bs,
    int idx,
    size_t capacity
) {
    bs->buffers_[idx] = malloc(sizeof(Vec*) * capacity);
    bs->sizes[idx] = 0;
    bs->capacities[idx] = capacity;
}


BufferStore* new_buffer_store(BufferCaps* caps)
{
    BufferStore* bs = calloc(1, sizeof(BufferStore));

    if (!caps) {
        caps = default_caps();
    }

    bs->sizes_[0] = BUFF_XXS;
    bs->sizes_[1] = BUFF_XS;
    bs->sizes_[2] = BUFF_S;
    bs->sizes_[3] = BUFF_M;
    bs->sizes_[4] = BUFF_L;
    bs->sizes_[5] = BUFF_XL;
    bs->sizes_[6] = BUFF_XXL;
    bs->sizes_[7] = BUFF_SU;


    bs->buffers_ = calloc(
        BUFFER_SIZE_CNT,
        sizeof(Vec**)
    );


    buffer_store_init_bucket(bs, 0, caps->xxs);
    buffer_store_init_bucket(bs, 1, caps->xs);
    buffer_store_init_bucket(bs, 2, caps->s);
    buffer_store_init_bucket(bs, 3, caps->m);
    buffer_store_init_bucket(bs, 4, caps->l);
    buffer_store_init_bucket(bs, 5, caps->xl);
    buffer_store_init_bucket(bs, 6, caps->xxl);
    buffer_store_init_bucket(bs, 7, caps->su);


    free(caps);

    return bs;
}


Vec* grab_buff(BufferStore* bs, uint16_t size)
{
    if (size == 0)
        return NULL;


    for (int i = 0; i < BUFFER_SIZE_CNT; i++) {

        if (size <= bs->sizes_[i]) {

            if (bs->sizes[i] == 0) {
                Vec* v = calloc(1, sizeof(Vec));

                v->b = malloc(size);
                v->cap = size;
                v->len = 0;
                v->c = v->b;

                return v;
            }


            Vec* v = bs->buffers_[i][bs->sizes[i] - 1];
            bs->sizes[i]--;

            return v;
        }
    }

    return NULL;
}


void put_buff(BufferStore* bs, Vec* v)
{
    if (!v)
        return;


    for (int i = 0; i < BUFFER_SIZE_CNT; i++) {

        if (v->cap == bs->sizes_[i]) {

            if (bs->sizes[i] < bs->capacities[i]) {

                v->len = 0;
                v->c = v->b;
                memset(v->b, 0, v->cap);

                bs->buffers_[i][bs->sizes[i]] = v;
                bs->sizes[i]++;

                return;
            }
        }
    }


    free(v->b);
    free(v);
}
