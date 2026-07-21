/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#include "ledger/ledger.h"
#include <sys/random.h>
#include "kzg/helpers.h"

extern "C" {

int ledger_open(
    void** out,
    const char* path, 
    size_t cache_size,
    size_t map_size,
    const char* tag,
    unsigned char* secret,
    size_t secret_size
) {
    if (!cache_size || !map_size) return ZERO_PARAMETER;
    if (!path || !tag) return NULL_PARAMETER;

    blst_scalar s;
    if (secret) {

        Hash secret_hash;
        derive_hash(secret_hash.h, {secret, secret_size});
        hash_to_sk(&s, secret_hash.h);
        std::memset(secret, 0, secret_size);
        std::memset(secret_hash.h, 0, sizeof(secret_hash.h));
    } else {
        byte random[32];
        assert(getrandom(random, 32, 0) == 32);
        hash_to_sk(&s, random);
        std::memset(random, 0, 32);
    }

    *out = new Ledger(path, cache_size, map_size, tag, s);

    return OK;
}

int ledger_get_SRS(
    void *ledger, 
    void** out,
    size_t* out_size
) {
    if (!ledger) return NULL_PARAMETER;
    auto l = reinterpret_cast<Ledger*>(ledger);

    size_t EXPECTED_SIZE = (
        BRANCH_ORDER * blst_p1_sizeof() + 
        BRANCH_ORDER * blst_p2_sizeof()
    );

    std::vector<blst_p1> g1s;
    std::vector<blst_p2> g2s;

    SRS* setup = &l->get_gadgets()->settings.setup;

    *out = malloc(EXPECTED_SIZE);

    byte* cursor = reinterpret_cast<byte*>(*out);

    for (auto &g1: setup->g1_powers_jacob) {
        blst_p1_compress(cursor, &g1);
        cursor += blst_p1_sizeof();
    }

    for (auto &g2: setup->g2_powers_jacob) {
        blst_p2_compress(cursor, &g2);
        cursor += blst_p2_sizeof();
    }

    return 0;
}

int ledger_set_SRS(
    void *ledger, 
    const unsigned char *setup, 
    size_t setup_size
) {
    if (!ledger || !setup) return NULL_PARAMETER;

    auto l = reinterpret_cast<Ledger*>(ledger);

    size_t EXPECTED_SIZE = (BRANCH_ORDER * blst_p1_sizeof() * 2);
    if (setup_size != EXPECTED_SIZE) return INVALID_SETUP_SIZE;

    std::vector<blst_p1> g1s;
    g1s.reserve(BRANCH_ORDER);

    std::vector<blst_p2> g2s;
    g2s.reserve(BRANCH_ORDER);

    const byte* cursor = setup;

    for (int i{}; i < BRANCH_ORDER; i++) {
        g1s.push_back(p1_from_bytes(cursor));
        cursor += blst_p1_sizeof();
    }

    for (int i{}; i < BRANCH_ORDER; i++) {
        g2s.push_back(p2_from_bytes(cursor));
        cursor += blst_p2_sizeof();
    }

    l->get_gadgets()->settings.setup.set_srs(g1s, g2s);

    return 0;
}
}
