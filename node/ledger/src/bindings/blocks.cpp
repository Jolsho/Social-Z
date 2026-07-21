/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#include "utils/bitmap.h"
#include "kzg/helpers.h"
#include "blocks/processing.h"
#include "sz/codec.h"

int ledger_finalize(
    void* ledger, 
    const HashT* block_hash, 
    void** out,
    size_t* out_size
) {
    if (!ledger || !block_hash) return NULL_PARAMETER;
    Hash bh;
    memcpy(bh.h, block_hash->b, HASH_SIZE);

    Hash h;
    int rc = finalize_block(*(Ledger*)ledger, &bh, &h);
    if (rc == 0) {

        *out = malloc(HASH_SIZE);
        *out_size = HASH_SIZE;

        std::memcpy(*out, h.h, HASH_SIZE);
    }
    return rc;
}

int ledger_prune(
    void* ledger,  
    const HashT* block_hash
) {
    if (!ledger || !block_hash) return NULL_PARAMETER;
    Hash bh;
    memcpy(bh.h, block_hash->b, HASH_SIZE);

    auto l = reinterpret_cast<Ledger*>(ledger);
    return prune_block(*l, &bh);
}

int ledger_justify(
    void* ledger,  
    const HashT* block_hash
) {
    if (!ledger || !block_hash) return NULL_PARAMETER;
    auto l = reinterpret_cast<Ledger*>(ledger);
    Hash bh;
    memcpy(bh.h, block_hash->b, HASH_SIZE);

    return justify_block(*l, &bh);
}

int ledger_generate_existence_proof(
    void* ledger, 
    const unsigned char* key, size_t key_size,
    uint8_t val_idx,
    void** out, 
    size_t* out_size,
    const HashT* block_hash = nullptr
) {
    if (!ledger || !key) return NULL_PARAMETER;
    if (val_idx < LEAF_ORDER) return VAL_IDX_RANGE;
    auto l = reinterpret_cast<Ledger*>(ledger);

    const ByteSlice key_slice((byte*)key, key_size);

    Hash key_hash;
    derive_hash(key_hash.h, key_slice);
    key_hash.h[31] = val_idx;

    std::vector<Commitment> Cs;
    std::vector<Proof> Pis;
    Bitmap<8> split_map{};

    Hash bh;
    memcpy(bh.h, block_hash->b, HASH_SIZE);

    int rc = generate_proof(*l, Cs, Pis, &split_map, &key_hash, &bh);
    if (rc != OK) return rc;

    size_t total_size;
    total_size += sizeof(uint8_t);
    total_size += (Cs.size() * sizeof(Commitment));
    total_size += sizeof(uint8_t);
    total_size += (Pis.size() * sizeof(Proof));
    total_size += sizeof(uint8_t); // split_map

    *out = malloc(total_size);
    *out_size = total_size;

    auto cursor = reinterpret_cast<byte*>(*out);

    *cursor++ = Cs.size();
    for (auto &commit: Cs) {
        blst_p1_compress(cursor, &commit);
        cursor += sizeof(Commitment);
    }

    *cursor++ = Pis.size();
    for (auto &proof: Pis) {
        blst_p1_compress(cursor, &proof);
        cursor += sizeof(Proof);
    }

    *cursor++ = *split_map.data_ptr();

    return 0;
}

int ledger_validate_proof(
    void* ledger, 
    const unsigned char* key, size_t key_size,

    const HashT* value_hash, uint8_t val_idx,

    const unsigned char* proof, size_t proof_size
) {
    if (!ledger || !key || !value_hash) return NULL_PARAMETER;
    if (val_idx < LEAF_ORDER) return VAL_IDX_RANGE;

    auto l = reinterpret_cast<Ledger*>(ledger);

    auto cursor = proof;

    uint8_t Cs_size = *cursor;
    cursor++;
    std::vector<Commitment> Cs(Cs_size);;
    for (auto &commit: Cs) {
        commit = p1_from_bytes(cursor);
        cursor += sizeof(Commitment);
    }


    uint8_t Pis_size = *cursor;
    cursor++;
    std::vector<Proof> Pis(Pis_size);
    for (auto &proof: Pis) {
        proof = p1_from_bytes(cursor);
        cursor += sizeof(Proof);
    }

    Bitmap<8> split_map(cursor++);

    const ByteSlice key_slice((byte*)key, key_size);
    Hash key_hash;
    derive_hash(key_hash.h, key_slice);
    key_hash.h[31] = val_idx;

    std::vector<size_t> Zs;
    std::vector<blst_scalar> Ys;

    Hash vh;
    memcpy(vh.h, value_hash->b, HASH_SIZE);

    derive_Zs_n_Ys(*l, &key_hash, &vh, &split_map, &Cs, &Pis, &Zs, &Ys);

    return valid_proof(*l, &Cs, &Pis, &split_map, &key_hash, &vh, val_idx);
}

