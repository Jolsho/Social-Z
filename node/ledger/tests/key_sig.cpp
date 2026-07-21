/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#include <cassert>
#include <cstdio>
#include "key_sig.h"
#include "blst.h"
#include "hashing.h"
#include "helpers.h"

void test_single_key_sig() {
    Hash seed = new_hash();
    seeded_hash(&seed, 1113);

    auto [msg, msg_len] = str_to_bytes("the_message");
    auto [dst, dst_len] = str_to_bytes("bullet_ledger");

    key_pair keys = gen_key_pair(dst, dst_len, seed);

    blst_p2 hash; 
    blst_hash_to_g2(&hash, msg, msg_len, dst, dst_len);
    blst_sign_pk_in_g1(&hash, &hash, &keys.sk);

    assert(verify_sig(
        keys.pk, hash, 
        msg, msg_len,
        dst, dst_len
    ));
    printf("SIGNATURE VALIDATED. \n");
    printf("\n");
}

void test_many_key_sig() {
    auto [msg, msg_len] = str_to_bytes("msg");
    auto [dst, dst_len] = str_to_bytes("bullet_ledger");

    blst_p2 hash;
    blst_hash_to_g2(&hash, msg, msg_len, dst, dst_len);

    blst_p2 agg_sig = new_p2();

    std::vector<blst_p1> pks;
    pks.reserve(5);
    Hash seed = new_hash();
    for (size_t i = 0; i < 5; i++) {

        seeded_hash(&seed, i);
        key_pair keys = gen_key_pair(dst, dst_len, seed);

        blst_p2 tmp_sig;
        blst_sign_pk_in_g1(&tmp_sig, &hash, &keys.sk);

        blst_p2_add_or_double(&agg_sig, &agg_sig, &tmp_sig);
        pks.push_back(keys.pk);
    }

    assert(verify_aggregate_signature(pks, agg_sig, msg, msg_len, dst, dst_len));
    printf("AGGREGATE_SIGNATURE VALIDATED. \n");
    printf("\n");
}

void main_key_sig() {
    printf("TESTING ONE KEY & SIGNATURE \n");
    test_single_key_sig();

    printf("\n");

    printf("TESTING MANY KEYS & SIGNATURES \n");
    test_many_key_sig();

    printf("=====================================\n");
}
