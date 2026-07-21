/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#pragma once
#include "blst.h"
#include "utils/hashing.h"
#include <array>
#include <vector>

using bytes32 = std::array<byte, 32>;

struct key_pair {
    blst_p1 pk;
    blst_scalar sk;
};

std::tuple<const byte*, size_t> str_to_bytes(const char* str);

key_pair gen_key_pair(
    const byte* tag, 
    size_t tag_len,
    Hash seed
);

bool verify_sig(
    const blst_p1 &PK,
    blst_p2 &signature, 
    const byte* msg,
    size_t msg_len,
    const byte* dst,
    size_t dst_len
);

bool verify_aggregate_signature(
    std::vector<blst_p1>& pks,
    const blst_p2& agg_sig,
    const byte* msg,
    size_t msg_len,
    const byte* dst,
    size_t dst_len
);



