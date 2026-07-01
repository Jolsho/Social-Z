#pragma once
#include "sz/api/actor.h"
#include "sodium/crypto_aead_chacha20poly1305.h"
#include "sz/utils/buffers.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <sodium/randombytes.h>

static constexpr const size_t ADLEN = 7;
static constexpr const unsigned char AD[ADLEN] { 's','o','c','i','a','l','z' };

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
    static constexpr size_t CODE_LEN    = sizeof(PktCode);

    static constexpr size_t TOO_OFF     = CODE_OFF + CODE_LEN;
    static constexpr size_t TOO_LEN     = sizeof(Actors);

public:
    inline bool is_done() { return done; }

    static constexpr size_t MAX_LEN     = BufferSize::SU;
    static constexpr size_t PREFIX_LEN  = LEN_LEN + VERSION_LEN + PUB_KEY_LEN + NONCE_LEN + TAG_LEN;

    uint64_t        prefix_cursor_ = 0;
    uint8_t*  prefix_[PREFIX_LEN];

    uint64_t        body_cursor_ = 0;
    Vec*            buff_ = nullptr;


    inline Packet(Vec* buff = nullptr) { buff_ = buff; }
    inline uint8_t* get_prefix_cursor() { return prefix_[prefix_cursor_]; }
    inline uint8_t* get_body_cursor() { return &buff_->b[body_cursor_]; }

    inline void get_len(uint64_t* len)  { memcpy(&len, prefix_ + LEN_OFF, LEN_LEN); }
    inline void set_len(uint64_t len)   { memcpy(prefix_ + LEN_OFF, &len, LEN_LEN); }

    inline void get_key(Key* key)       { memcpy(key->b, prefix_ + PUB_KEY_OFF, PUB_KEY_LEN); }
    inline void set_key(const Key& key) { memcpy(prefix_ + PUB_KEY_OFF, key.b, KEY_SIZE); }

    inline void get_version(uint64_t* v) { memcpy(&v, prefix_ + VERSION_OFF, VERSION_LEN); }
    inline void set_version(uint64_t v) { memcpy(prefix_ + VERSION_OFF, &v, VERSION_LEN); }

    inline void get_nonce(Nonce& n)     { memcpy(&n, prefix_ + NONCE_OFF, NONCE_LEN); }
    inline void new_nonce()             { randombytes_buf(prefix_ + NONCE_OFF, NONCE_LEN); }

    inline uint8_t* get_tag()     { return prefix_[TAG_OFF]; }

    inline void get_code(PktCode* c)    { memcpy(&c, prefix_ + CODE_OFF, CODE_LEN); }
    inline void set_code(PktCode c)     { memcpy(prefix_ + CODE_OFF, &c, CODE_LEN); }

    inline void get_too(Actors* c)     { memcpy(&c, prefix_ + TOO_OFF, TOO_LEN); }
    inline void set_too(Actors c)      { memcpy(prefix_ + TOO_OFF, &c, TOO_LEN); }

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

        uint64_t len;
        get_len(&len);

        int r = crypto_aead_chacha20poly1305_decrypt_detached(
            buff_->b, NULL,
            buff_->b, len, 
            get_tag(),
            AD, ADLEN, 
            nonce.b, 
            rx_key.b
        );
        if (r != 0) return r;

        done = true;

        return 0;
    }

    int encrypt_body(Key& tx_key) {
        if (!buff_) return 0;

        Nonce nonce;
        randombytes_buf(nonce.b, NONCE_LEN);
        memcpy(&buff_->b + NONCE_OFF, nonce.b, NONCE_LEN);
        unsigned long long mac_len = TAG_LEN;

        uint64_t len;
        get_len(&len);

        int r = crypto_aead_chacha20poly1305_encrypt_detached(
            buff_->b, 
            get_tag(), &mac_len,
            buff_->b, len,
            AD, ADLEN, 
            NULL,
            nonce.b, tx_key.b
        );
        if (r != 0) return r;
        done = true;
        return 0;
    }
};
