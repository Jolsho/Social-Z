/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#pragma once
#include "ledger/alloc.h"

struct Gadgets {
    KZGSettings settings;
    NodeAllocator alloc;

    Gadgets(
        size_t degree, 
        const blst_scalar &s, 
        std::string tag,
        std::string path,
        size_t cache_size,
        size_t map_size
    ) : 
        settings(init_settings(degree, s, tag)),
        alloc(path, cache_size, map_size)
    {}
};

using Gadgets_ptr = std::shared_ptr<Gadgets>;

Gadgets_ptr init_gadgets(
    size_t degree, 
    const blst_scalar &s, 
    std::string tag,
    std::string path,
    size_t cache_size,
    size_t map_size
);
