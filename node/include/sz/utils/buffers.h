/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once 
#include "sz/api/vec.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t BufferSize; 
#define BUFF_XXS    ((BufferSize)512)
#define BUFF_XS     ((BufferSize)1024)
#define BUFF_S      ((BufferSize)2048)
#define BUFF_M      ((BufferSize)4096)
#define BUFF_L      ((BufferSize)8192)
#define BUFF_XL     ((BufferSize)16384)
#define BUFF_XXL    ((BufferSize)32768)
#define BUFF_SU     ((BufferSize)65386)

#define BUFFER_SIZE_CNT 8

typedef struct BufferCaps {
    size_t xxs;
    size_t xs;
    size_t s;
    size_t m;
    size_t l;
    size_t xl;
    size_t xxl;
    size_t su;
} BufferCaps;

typedef struct {
    BufferSize sizes_[BUFFER_SIZE_CNT];

    Vec*** buffers_;
    size_t sizes[BUFFER_SIZE_CNT];
    size_t capacities[BUFFER_SIZE_CNT];

} BufferStore;
BufferCaps* default_caps();
BufferStore* new_buffer_store(BufferCaps*);
Vec* grab_buff(BufferStore* bs, uint16_t size);
void put_buff(BufferStore* bs, Vec* v);

#ifdef __cplusplus
}
#endif
