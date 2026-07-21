/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#pragma once
#include "kzg/kzg.h"
#include "ledger/ledger.h"

int finalize_block(
    Ledger &ledger, 
    const Hash* block_hash,
    Hash* out
);

int prune_block(
    Ledger &ledger, 
    const Hash* block_hash
);

int justify_block(
    Ledger &ledger, 
    const Hash* block_hash
);

int generate_proof(
    Ledger &ledger, 
    std::vector<Commitment> &Cs,
    std::vector<Proof> &Pis,
    Bitmap<8>* split_map,
    const Hash* key_hash,
    const Hash* block_hash = nullptr
);

bool valid_proof(
    Ledger &ledger,
    std::vector<Commitment>* Cs,
    std::vector<Proof>* Pis,
    Bitmap<8>* split_map,
    const Hash* key_hash,
    const Hash* val_hash,
    const uint8_t val_idx,
    const Hash* block_hash = nullptr
);

void derive_Zs_n_Ys(
    Ledger &ledger,
    const Hash* key_hash,
    const Hash* val_hash,
    Bitmap<8>* split_map,
    std::vector<Commitment>* Cs,
    std::vector<Proof>* Pis,
    std::vector<size_t>* Zs,
    Scalar_vec* Ys
);
