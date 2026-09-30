/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/hash.h"

Hasher new_hasher(void) {
    Hasher h;
    blake3_hasher_init(&h.inner);
    return h;
}

void hash_update(Hasher* h, const uint8_t* data, size_t size) {
    blake3_hasher_update(&h->inner, data, size);
}

HashT hash_finalize(Hasher* h) {
    HashT hash;
    blake3_hasher_finalize(&h->inner, hash.b, HASH_SIZE);
    return hash;
}

