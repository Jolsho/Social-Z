#pragma once
#include "utils/hash.h"
#include "sodium/crypto_sign.h"
#include <sodium/crypto_aead_chacha20poly1305.h>
#include <sodium/crypto_secretstream_xchacha20poly1305.h>
#include <string>
#include <array>


using Key = std::array<unsigned char, crypto_secretstream_xchacha20poly1305_KEYBYTES>;
static constexpr size_t KEY_SIZE = sizeof(Key);
const Key ZERO_KEY = {0};

using Nonce = std::array<unsigned char, crypto_aead_chacha20poly1305_NPUBBYTES>;
static constexpr size_t NONCE_SIZE = sizeof(Nonce);

using Signature = std::array<std::byte, crypto_sign_BYTES>;
static constexpr size_t SIG_SIZE = sizeof(Signature);

std::string key_to_str(Key key);
size_t str_to_key(const char* key_str, Key& key);

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

inline bool valid_signature(Key& signer, Signature& sig, Hash& hash) {
    return crypto_sign_verify_detached(
        reinterpret_cast<const unsigned char*>(sig.data()), 
        reinterpret_cast<const unsigned char*>(hash.data()), HASH_SIZE, 
        reinterpret_cast<const unsigned char*>(signer.data())
    );
}
