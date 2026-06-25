#pragma once
#include "api/types.h"
#include <cstring>
#include <sodium/crypto_aead_chacha20poly1305.h>
#include <sodium/crypto_secretstream_xchacha20poly1305.h>
#include <string>
#include "sodium/crypto_sign.h"

inline bool valid_signature(Key& signer, Signature& sig, HashT& hash) {
    return crypto_sign_verify_detached(sig.b, hash.b, HASH_SIZE, signer.b);
}

std::string key_to_str(Key& key);

int str_to_key(const char* key_str, Key& key);

struct KeyHash {
    std::size_t operator()(const Key& k) const {
        std::size_t h = 0;
        for (int i = 0; i < KEY_SIZE; i++) {
            h = h * 31 + reinterpret_cast<uint8_t>(k.b[i]);
        }
        return h;
    }
};

struct KeyCompare {
    bool operator()(const Key& a, const Key& b) const {
        return memcmp(a.b, b.b, KEY_SIZE) <= 0;
    }
};

struct KeyEqual {
    bool operator()(const Key& a, const Key& b) const {
        return memcmp(a.b, b.b, KEY_SIZE) == 0;
    }
};



struct KeyPair {
    Key priv;
    Key pub;
};

int new_keypair(KeyPair& keys);

