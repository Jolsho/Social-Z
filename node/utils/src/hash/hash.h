#pragma once
#include "blake3.h"
#include "api/types.h"
#include <cstddef>
#include <cstring>

class Hasher {
    blake3_hasher self;

public:
    Hasher() {
        blake3_hasher_init(&self);
    }

    void update(const unsigned char* data, size_t size) {
        blake3_hasher_update(&self, data, size);
    }

    HashT finalize() {
        HashT hash;
        blake3_hasher_finalize(
            &self, (uint8_t*)hash.b, HASH_SIZE
        );
        return hash;
    }
};

inline bool operator==(const HashT& h1, const HashT& h2) {
    return memcmp(h1.b, h2.b, HASH_SIZE) == 0;

}

static constexpr HashT ZERO_HASH {};
