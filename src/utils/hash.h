#pragma once
#include "blake3.h"
#include "crypto.h"
#include <cstddef>

class Hasher {
    blake3_hasher self;

public:
    Hasher() {
        blake3_hasher_init(&self);
    }

    void update(const std::byte* data, size_t size) {
        blake3_hasher_update(&self, data, size);
    }

    Hash finalize() {
        Hash hash;
        blake3_hasher_finalize(
            &self, (uint8_t*)hash.data(), hash.size()
        );
        return hash;
    }
};
