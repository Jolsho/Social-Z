/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_common/codec.h"
#include <cstring>

struct KeyHash {
    size_t operator()(const Key& k) const {
        size_t h = 0;
        for (int i = 0; i < KEY_SIZE; i++) {
            h = h * 31 + reinterpret_cast<uint8_t>(k.b[i]);
        }
        return h;
    }
};

struct KeyCompare {
    bool operator()(const Key& a, const Key& b) const {
        return memcmp(a.b, b.b, KEY_SIZE) <= 0;
    }
};

inline bool operator==(const Key& k1, const Key& k2) {
    return memcmp(k1.b, k2.b, HASH_SIZE) == 0;

}
