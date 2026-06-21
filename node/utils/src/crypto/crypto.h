#pragma once
#include <sodium/crypto_aead_chacha20poly1305.h>
#include <sodium/crypto_secretstream_xchacha20poly1305.h>
#include <string>
#include <array>
#include "utils/key.h"


using Nonce = std::array<unsigned char, crypto_aead_chacha20poly1305_NPUBBYTES>;
static constexpr size_t NONCE_SIZE = sizeof(Nonce);

std::string key_to_str(Key key);
int str_to_key(const char* key_str, Key& key);

struct KeyHash {
    std::size_t operator()(const Key& k) const {
        std::size_t h = 0;
        for (unsigned char c : k) {
            h = h * 31 + c;
        }
        return h;
    }
};

struct KeyPair {
    Key priv;
    Key pub;
};

int new_keypair(KeyPair& keys);

