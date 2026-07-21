/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include "blst.h"
#include "utils/hashing.h"

// 13 bytes for path, 1 for level, and 2 for block_id
const size_t ID_SIZE = 13 + 1 + 2;

class NodeId {
private:
    byte buff_[ID_SIZE];
public:
    NodeId();
    NodeId(const NodeId* other);
    NodeId(const Hash* key, uint8_t level, uint16_t block_id);
    NodeId(const std::vector<byte>* key, uint16_t block_id);

    bool operator ==(const NodeId& other) const noexcept;
    std::string to_string() const;

    ~NodeId();

    uint16_t get_block_id() const;
    void set_block_id(uint16_t id);

    uint8_t get_level() const;
    void set_level(uint8_t level);
    void increment_level();

    void set_child_nibble(byte nib);
    void set_self_nibble(byte nib);

    const byte* get_full() const;
    std::array<byte, ID_SIZE> get_full_array() const;

    int cmp(const Hash* b);
    size_t size() const;
};


struct NodeIdHash {
    size_t operator()(const NodeId& k) const noexcept {
        auto array = k.get_full_array();
        uint64_t h = 1469598103934665603ull; // FNV-1a
        for (byte b : array) {
            h ^= static_cast<unsigned char>(b);
            h *= 1099511628211ull;
        }
        return static_cast<size_t>(h);
    }
};

