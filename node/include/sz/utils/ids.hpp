/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include <stdint.h>
#include <vector>

class Ids {
    std::vector<uint16_t> free;
    uint16_t next = 0;

public:
    Ids(uint16_t cap) {
        free.reserve(cap);
        if (cap > 0) {
            next = 1;
        }
    }

    inline void next_id(uint16_t* i) {
        if (free.size() == 0) {
            *i = next++;
        } else {
            *i = free.back();
            free.pop_back();
        }
    }

    inline void put_back_id(uint16_t i) {
        if (i + 1 == next) {
            next = i;
        } else {
            free.push_back(i);
        }
    }
};
