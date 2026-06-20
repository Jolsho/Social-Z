#pragma once
#include "sodium/crypto_aead_chacha20poly1305.h"
#include "utils/buffers.h"
#include "crypto.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <sodium/randombytes.h>

static constexpr const size_t ADLEN = 7;
static constexpr const unsigned char AD[ADLEN] { 'S','o','c','i','a','l','z' };

class Packet {

private:
    bool done = false;

    // PREFIX 
    static constexpr size_t LEN_OFF     = 0;
    static constexpr size_t LEN_LEN     = sizeof(uint64_t);

    static constexpr size_t VERSION_OFF = LEN_OFF + LEN_LEN;
    static constexpr size_t VERSION_LEN = sizeof(uint64_t);

    static constexpr size_t PUB_KEY_OFF = VERSION_OFF + VERSION_LEN;
    static constexpr size_t PUB_KEY_LEN = sizeof(Key);

    static constexpr size_t NONCE_OFF   = PUB_KEY_OFF + PUB_KEY_LEN;
    static constexpr size_t NONCE_LEN   = sizeof(Nonce);

    static constexpr size_t TAG_OFF     = NONCE_OFF + NONCE_LEN;
    static constexpr size_t TAG_LEN     = crypto_aead_chacha20poly1305_ABYTES;

    static constexpr size_t CODE_OFF    = TAG_OFF + TAG_LEN;
    static constexpr size_t CODE_LEN    = sizeof(int);

    static constexpr size_t FLAGS_OFF    = CODE_OFF + CODE_LEN;
    static constexpr size_t FLAGS_LEN    = sizeof(uint8_t);


public:
    inline bool is_done() { return done; }

    static constexpr size_t MAX_LEN     = BufferSize::SU;
    static constexpr size_t PREFIX_LEN  = LEN_LEN + VERSION_LEN + PUB_KEY_LEN + NONCE_LEN + TAG_LEN;

    uint64_t        prefix_cursor_ = 0;
    unsigned char*  prefix_[PREFIX_LEN];

    uint64_t        body_cursor_ = 0;
    Vec*            buff_ = nullptr;


    inline Packet(Vec* buff = nullptr) {
        buff_ = buff;
    }

    inline unsigned char* get_prefix_cursor() {
        return prefix_[prefix_cursor_];
    }


    inline unsigned char* get_body_cursor() {
        return &buff_->b[body_cursor_];
    }

    uint64_t get_len() {
        uint64_t len;
        memcpy(&len, prefix_ + LEN_OFF, LEN_LEN);
        return len;
    }
    inline void set_len(uint64_t len) {
        memcpy(prefix_ + LEN_OFF, &len, LEN_LEN);
    }

    Key get_key() {
        Key key;
        memcpy(key.data(), prefix_ + PUB_KEY_OFF, PUB_KEY_LEN);
        return key;
    }
    inline void set_key(const Key& key) {
        memcpy(prefix_ + PUB_KEY_OFF, key.data(), key.size());
    }

    uint64_t get_version() {
        uint64_t v;
        memcpy(&v, prefix_ + VERSION_OFF, VERSION_LEN);
        return v;
    }
    inline void set_version(uint64_t v) {
        memcpy(prefix_ + VERSION_OFF, &v, VERSION_LEN);
    }

    inline void get_nonce(Nonce& n) {
        memcpy(&n, prefix_ + NONCE_OFF, NONCE_LEN);
    }
    inline void new_nonce() {
        randombytes_buf(prefix_ + NONCE_OFF, NONCE_LEN);
    }

    inline unsigned char* get_tag() { return prefix_[TAG_OFF]; }

    int get_code() {
        int c;
        memcpy(&c, prefix_ + CODE_OFF, CODE_LEN);
        return c;
    }
    inline void set_code(uint64_t c) {
        memcpy(prefix_ + CODE_OFF, &c, CODE_LEN);
    }

    static constexpr uint8_t PING = 1;
    static constexpr uint8_t PONG = 2;
    void mark_as_ping() {
        memcpy(prefix_ + FLAGS_OFF, &PING, FLAGS_LEN);
    }
    bool is_ping() {
        uint8_t flag;
        memcpy(&flag, prefix_ + FLAGS_OFF, FLAGS_LEN);
        return flag == PING;
    }
    void mark_as_pong() {
        memcpy(prefix_ + FLAGS_OFF, &PONG, FLAGS_LEN);
    }
    bool is_pong() {
        uint8_t flag;
        memcpy(&flag, prefix_ + FLAGS_OFF, FLAGS_LEN);
        return flag == PONG;
    }


    void wipe() {
        memset(&buff_->b, 0, buff_->cap);
        body_cursor_    = 0;
        prefix_cursor_  = 0;
        done = false;
    }

    int decrypt_body(Key& rx_key) {
        if (!buff_) return 0;

        Nonce nonce;
        get_nonce(nonce);

        int r = crypto_aead_chacha20poly1305_decrypt_detached(
            buff_->b, NULL,
            buff_->b, get_len(), 
            get_tag(),
            AD, ADLEN, 
            nonce.data(), 
            rx_key.data()
        );
        if (r != 0) return r;

        done = true;

        return 0;
    }

    int encrypt_body(Key& tx_key) {
        if (!buff_) return 0;

        Nonce nonce;
        randombytes_buf(nonce.data(), NONCE_LEN);
        memcpy(&buff_->b + NONCE_OFF, nonce.data(), NONCE_LEN);
        unsigned long long mac_len = TAG_LEN;

        int r = crypto_aead_chacha20poly1305_encrypt_detached(
            buff_->b, 
            get_tag(), &mac_len,
            buff_->b, get_len(),
            AD, ADLEN, 
            NULL,
            nonce.data(), tx_key.data()
        );
        if (r != 0) return r;
        done = true;
        return 0;
    }
};
