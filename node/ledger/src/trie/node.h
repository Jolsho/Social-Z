/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#pragma once
#include "utils/bitmap.h"
#include "trie/nodeid.h"
#include "trie/state_types.h"
#include "kzg/polynomial.h"
#include <memory>

class Node;
using Node_ptr = std::shared_ptr<Node>;

class Node {
public:
    virtual ~Node() = default;

    virtual const NodeId* get_id() const = 0;
    virtual void set_id(const NodeId* id) = 0;

    virtual const Commitment* get_commitment() const = 0;
    virtual void set_commitment(const Commitment &c) = 0;
    virtual const Commitment* derive_commitment() = 0;

    virtual bool should_delete() const = 0;

    virtual const NodeId* get_next_id(byte nib) = 0;
    virtual std::vector<byte> to_bytes() const = 0;

    virtual int put(
        const Hash* key,
        const Hash* val_hash,
        uint16_t block_id
    ) = 0;

    virtual int replace(
        const Hash* key,
        const Hash* val_hash,
        const Hash* prev_val_hash,
        uint16_t block_id
    ) = 0;

    virtual int remove(
        const Hash* key,
        uint16_t block_id
    ) = 0;

    virtual int create_account(
        const Hash* key,
        uint16_t block_id
    ) = 0;

    virtual int delete_account(
        const Hash* key,
        uint16_t block_id
    ) = 0;

    virtual int generate_proof(
        const Hash* key,
        std::vector<Polynomial> &Fxs,
        std::vector<blst_p1> &Cs,
        Bitmap<8>* split_map
    ) = 0;


    virtual int finalize(
        const Hash* shard_path,
        uint16_t block_id,
        Commitment *out,
        size_t start = 0, 
        size_t end = 0,
        Polynomial* Fx = nullptr
    ) = 0;

    virtual int prune(uint16_t block_id) = 0;

    virtual int justify(uint16_t block_id) = 0;

    virtual bool commit_is_in_path(
        const Hash* key,
        const Commitment &commitment
    ) = 0;
};

