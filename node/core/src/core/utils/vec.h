#pragma once
#include "bindings.h"
#include <cstring>
#include <type_traits>

inline bool vec_write(Vec* dst, const unsigned char* src, size_t n) {
    if (dst->len + n > dst->cap) return false;
    memcpy(dst->c, src, n);
    dst->len += n;
    dst->c += n;
    return true;
}


inline bool vec_read(Vec* src, unsigned char* dst, size_t n) {
    if (((src->c - src->b) - src->len) < n) return false;
    memcpy(dst, src->c, n);
    src->c += n;
    return true;
}

template<typename T>
inline bool vec_write(Vec* dst, const T& src) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (dst->len + sizeof(T) > dst->cap) return false;
    memcpy(dst->c, &src, sizeof(T));
    dst->len += sizeof(T);
    dst->c += sizeof(T);
    return true;
}

template<typename T>
inline T vec_read(Vec* src) {
    static_assert(std::is_trivially_copyable_v<T>);
    T t {};
    if (((src->c - src->b) - src->len) < sizeof(T)) return t;
    memcpy(&t, src->c, sizeof(T));
    src->c += sizeof(T);
    return t;
}

template<typename T>
inline bool vec_read(Vec* src, T& dst) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (((src->c - src->b) - src->len) < sizeof(T)) return false;
    memcpy(&dst, src->c, sizeof(T));
    src->c += sizeof(T);
    return true;
}

inline size_t vec_remaining(Vec* v) { return v->len - (v->c - v->b); }

inline void vec_shift_remaining(Vec* v) {
    memmove(v->b, v->c, v->len - (v->c - v->b));
}


inline bool vec_write_str(Vec* dst, const char* src, uint64_t n = 0) {
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
