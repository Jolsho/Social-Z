#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct Vec {
    uint8_t*    b;
    uint8_t*    c;
    size_t      len;
    size_t      cap;
} Vec;

Vec* new_vec(size_t cap);
void free_vec(Vec* v);

#ifdef __cplusplus
}
#endif
