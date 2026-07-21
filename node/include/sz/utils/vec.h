/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz/api/vec.h"
#include <memory.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

inline bool vec_write(Vec* dst, const void* src, size_t n) {
    if (dst->len + n > dst->cap) return false;
    memcpy(dst->c, src, n);
    dst->len += n;
    dst->c += n;
    return true;
}


inline bool vec_write_c_str(Vec* dst, const char* src) {
    size_t len = strlen(src);
    if (dst->len + len > dst->cap) return false;
    memcpy(dst->c, src, len);
    dst->len += len;
    dst->c += len;
    return true;
}

inline bool vec_read(Vec* src, void* dst, size_t n) {
    if (((src->c - src->b) - src->len) < n) return false;
    memcpy(dst, src->c, n);
    src->c += n;
    return true;
}

inline size_t vec_remaining(Vec* v) { return v->len - (v->c - v->b); }

inline void vec_shift_remaining(Vec* v) {
    memmove(v->b, v->c, v->len - (v->c - v->b));
}


inline bool vec_write_str_len_pre(Vec* dst, const char* src, uint64_t n) {
    if (n == 0) {
        n = strlen(src);
        if (n == 0) return true;
    }
    if (dst->len + n + sizeof(uint64_t) > dst->cap) return false;
    memcpy(dst->c, &n, sizeof(uint64_t));
    
    dst->len += sizeof(uint64_t);
    dst->c += sizeof(uint64_t);

    memcpy(dst->c, src, n);
    dst->len += n;
    dst->c += n;

    return true;
}

#ifdef __cplusplus
}
#endif
