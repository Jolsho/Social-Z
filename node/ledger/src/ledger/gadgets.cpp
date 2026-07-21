/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#include "ledger/gadgets.h"

Gadgets_ptr init_gadgets(
    size_t degree, 
    const blst_scalar &s, 
    std::string tag,
    std::string path,
    size_t cache_size,
    size_t map_size
) {
    auto g = std::make_shared<Gadgets>(
        degree, s, tag, path, cache_size, map_size
    );
    g->alloc.set_gadgets(g);
    return g;
}
