/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_common/codec.h"
#include <stdbool.h>

#ifdef __cplusplus
#include <cstring>
extern "C" {
#endif

typedef struct Hasher {
    void*   inner;
} Hasher;

Hasher new_hasher();
void hash_update(Hasher* hr, const uint8_t* data, size_t size);
HashT hash_finalize(Hasher* hr);

static inline bool is_zero_hash(const HashT* h) { 
    for (int i = 0; i < HASH_SIZE; i++) {
        if (h->b[i] != 0) return false;
    }
    return true;
}

static inline bool hash_is_equal(const HashT* h1, const HashT* h2) { 
    return memcmp(h1, h2, HASH_SIZE) == 0;
}

#ifdef __cplusplus
}

inline bool operator==(const HashT& h1, const HashT& h2) {
    return memcmp(h1.b, h2.b, HASH_SIZE) == 0;
}
#endif
