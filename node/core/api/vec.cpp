#include "sz/api/vec.h"
#include <cstdlib>

Vec* new_vec(size_t cap) {
    uint8_t* bytes = reinterpret_cast<uint8_t*>(malloc(cap));
    return new Vec{
        .b = bytes,
        .c = bytes,
        .len = 0,
        .cap = cap
    };
}

void free_vec(Vec* v) {
    if (!v) return;
    if (v->b != nullptr) free(v->b);
    delete v;
}
