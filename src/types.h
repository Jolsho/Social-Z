#pragma once
#include <sodium/crypto_aead_chacha20poly1305.h>
#include <sodium/crypto_secretstream_xchacha20poly1305.h>
#include <string>
#include <array>

using Key = std::array<unsigned char, crypto_secretstream_xchacha20poly1305_KEYBYTES>;
const Key ZERO_KEY = {0};
using Nonce = unsigned char[crypto_aead_chacha20poly1305_NPUBBYTES];

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

enum Actors : uint8_t {
    NETWORKER = 0,
    FILESYS = 1,
    RPC_SERVER = 2,
    BLOCKCHAIN = 3,

    COUNT,
    NONE,
};

using ConnID = uint16_t;

template<typename T>
T read_from_cursor(std::byte*& cursor) {
    T value;
    memcpy(&value, cursor, sizeof(T));
    cursor += sizeof(T);
    return value;
}

template<typename T>
void cpy_from_cursor(std::byte*& cursor, T* dst) {
    memcpy(dst, cursor, sizeof(T));
    cursor += sizeof(T);
}
