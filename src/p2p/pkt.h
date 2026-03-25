#pragma once
#include "sodium/crypto_aead_chacha20poly1305.h"
#include "utils/keys.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <sodium/randombytes.h>

class Packet {

private:

    std::byte*      buff_ = nullptr;
    uint64_t        capacity_ = 0;

    // PREFIX 
    static constexpr size_t LEN_OFF = 0;
    static constexpr size_t LEN_SZ = sizeof(uint64_t);

    static constexpr size_t VERSION_OFF = LEN_OFF + LEN_SZ;
    static constexpr size_t VERSION_SZ = sizeof(uint8_t);

    static constexpr size_t PUB_KEY_OFF = VERSION_OFF + VERSION_SZ;
    static constexpr size_t PUB_KEY_SZ = sizeof(Key);

    static constexpr size_t NONCE_OFF = PUB_KEY_OFF + PUB_KEY_SZ;
    static constexpr size_t NONCE_SZ = sizeof(Nonce);


    // HEADER
    static constexpr size_t CODE_OFF = NONCE_OFF + NONCE_SZ;
    static constexpr size_t CODE_SZ = sizeof(uint16_t);

    //BODY
    static constexpr size_t BODY_OFF = CODE_OFF + CODE_SZ;

public:

    static constexpr size_t MAX_LEN    = 2048;
    static constexpr size_t CHA_AD_LEN = 16;
    static constexpr size_t HEADER_LEN = CODE_SZ;
    static constexpr size_t PREFIX_LEN = NONCE_OFF + NONCE_SZ;
    static constexpr size_t BODY_SZ = MAX_LEN - (PREFIX_LEN + HEADER_LEN + CHA_AD_LEN);

    Packet() {
        buff_ = reinterpret_cast<std::byte*>(malloc(MAX_LEN));
        capacity_ = MAX_LEN;
    }
    ~Packet() {
        free(buff_);
        capacity_ = 0;
    }

    inline uint64_t cap() { return capacity_; }

    uint64_t    cursor_ = 0;
    std::byte* get_cursor() {
        return &buff_[cursor_];
    }

    uint64_t get_body_len() {
        uint64_t len;
        memcpy(&len, buff_ + LEN_OFF, LEN_SZ);
        return len;
    }
    inline void set_len(uint64_t len) {
        memcpy(buff_ + LEN_OFF, &len, LEN_SZ);
    }

    Key get_key() {
        Key key;
        memcpy(key.data(), buff_ + PUB_KEY_OFF, PUB_KEY_SZ);
        return key;
    }
    void set_key(const Key& key) {
        memcpy(buff_ + PUB_KEY_OFF, key.data(), key.size());
    }

    uint8_t get_version() {
        uint8_t v;
        memcpy(&v, buff_ + VERSION_OFF, VERSION_SZ);
        return v;
    }
    void set_version(uint8_t v) {
        memcpy(buff_ + VERSION_OFF, &v, VERSION_SZ);
    }

    inline void get_nonce(Nonce& n) {
        memcpy(&n, buff_ + NONCE_OFF, NONCE_SZ);
    }
    inline void new_nonce() {
        randombytes_buf(buff_ + NONCE_OFF, NONCE_SZ);
    }

    uint16_t get_code() {
        uint16_t c;
        memcpy(&c, buff_ + CODE_OFF, CODE_SZ);
        return c;
    }
    inline void set_code(uint16_t c) {
        memcpy(buff_ + CODE_OFF, &c, CODE_SZ);
    }

    inline std::byte* body() {
        return &buff_[BODY_OFF];
    }

    void wipe() {
        memset(&buff_, 0, capacity_);
        cursor_ = 0;
    }

    inline size_t expected_length() {
        return PREFIX_LEN + HEADER_LEN + get_body_len() + CHA_AD_LEN;
    }

    int decrypt_body(Key& rx_key) {
        Nonce nonce;
        get_nonce(nonce);
        unsigned char* enc_boundary = reinterpret_cast<unsigned char*>(body());
        unsigned long long len = get_body_len();

        return crypto_aead_chacha20poly1305_decrypt(
            enc_boundary, &len, NULL,
            enc_boundary, len + CHA_AD_LEN,
            NULL, 0, nonce.data(), rx_key.data()
        );
    }
    int encrypt_body(Key& tx_key) {

        unsigned long long len = get_body_len() ;
        unsigned long long clen = len + CHA_AD_LEN;
        unsigned char* enc_boundary = reinterpret_cast<unsigned char*>(body());

        Nonce nonce;
        randombytes_buf(nonce.data(), NONCE_SZ);
        memcpy(&buff_ + NONCE_OFF, nonce.data(), NONCE_SZ);

        return crypto_aead_chacha20poly1305_encrypt(
            enc_boundary, &clen,
            enc_boundary, len,
            NULL, 0, NULL,
            nonce.data(), tx_key.data()
       );
    }
};
