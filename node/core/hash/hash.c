/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "blake3.h"
#include <malloc.h>
#include "sz/hash.h"

Hasher new_hasher() {
    Hasher h;
    blake3_hasher_init((blake3_hasher*)h.inner);
    return h;
}

void hash_update(Hasher* h, const uint8_t* data, size_t size) {
    blake3_hasher_update((blake3_hasher*)h->inner, data, size);
}

HashT hash_finalize(Hasher* h) {
    HashT hash;
    blake3_hasher_finalize((blake3_hasher*)h->inner, hash.b, HASH_SIZE);
    return hash;
}


