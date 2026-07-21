/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "blake3.h"
#include "blst.h"
#include <cstdlib>
#include <cstring>
#include <span>
#include <string>

struct Hash {
    byte h[32];

    bool operator==(const Hash& other) const noexcept {
        return std::memcmp(h, other.h, sizeof(h)) == 0;
    }
};

using ByteSlice = std::span<byte>;

class BlakeHasher {
private: blake3_hasher h_;
public:
    BlakeHasher() { blake3_hasher_init(&h_); }
    ~BlakeHasher() = default;

    void update(const byte* data, const size_t size) {
        blake3_hasher_update(&h_, data, size);
    }
    void finalize(byte* out) {
        blake3_hasher_finalize(&h_, static_cast<uint8_t*>(out), 32);
    }
};

void derive_kv_hash(Hash out, const Hash &key_hash, const Hash &val_hash);

void derive_hash(byte* out, const ByteSlice &value);

Hash new_hash(const byte* h = nullptr);
void print_hash(const Hash &hash);
void seeded_hash(Hash* out, int i);
void hash_p1_to_scalar(const blst_p1* p1, blst_scalar* s, const std::string* tag);

struct HashHash {
    size_t operator()(const Hash& h) const noexcept {
        uint64_t h1 = 1469598103934665603ull; // FNV-1a
        for (byte b : h.h) {
            h1 ^= static_cast<unsigned char>(b);
            h1 *= 1099511628211ull;
        }
        return static_cast<size_t>(h1);
    }
};
