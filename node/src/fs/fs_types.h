#pragma once
#include "utils/sig.h"
#include "utils/buffers.h"
#include "utils/hash.h"
#include <array>
#include <cstdio>
#include <cstring>

struct Perm {
    static constexpr size_t DATA_SIZE = 256;

    Key         giver;
    Key         recipient;
    Nonce       nonce;
    std::array<unsigned char, DATA_SIZE>   data;
    Signature   signature;

    HashT hash() {
        Hasher h {};
        h.update(giver.data(), KEY_SIZE);
        h.update(recipient.data(), KEY_SIZE);
        h.update(nonce.data(), NONCE_SIZE);
        h.update(data.data(), DATA_SIZE);
        return h.finalize();
    }
};
static_assert(std::is_trivially_copyable_v<Perm>);
static constexpr size_t PERM_SZ = sizeof(Perm);

struct Voucher {
    static constexpr size_t TIME_SIZE = sizeof(time_t);

    Key         to;
    Key         from;

    std::array<unsigned char, Perm::DATA_SIZE> data {0};
    HashT       file_hash;
    time_t      expiration;

    Signature   signature;

    HashT hash() {
        Hasher h {};
        h.update(to.data(), KEY_SIZE);
        h.update(from.data(), KEY_SIZE);
        h.update(file_hash.b, HASH_SIZE);

        auto raw = reinterpret_cast<const unsigned char*>(&expiration);
        h.update(raw, TIME_SIZE);
        h.update(data.data(), Perm::DATA_SIZE);

        return h.finalize();
    }
};
static constexpr size_t VOUCHER_SZ = sizeof(Voucher);
static_assert(std::is_trivially_copyable_v<Voucher>);


struct PendingPermBucket {
    time_t          expires;
    Vec*            buff;

    inline void take_next(HashT& h) {
        memcpy(&h, buff->c - HASH_SIZE, HASH_SIZE);
        buff->len -= HASH_SIZE;
        buff->c -= HASH_SIZE;
    }

    inline bool put_next(HashT& h) {
        if (buff->len + HASH_SIZE > buff->cap) return false;
        memcpy(buff->c, &h, HASH_SIZE);
        buff->len += HASH_SIZE;
        buff->c += HASH_SIZE;
        return true;
    }

    inline bool is_empty() { return buff->len < HASH_SIZE; };

    inline bool remove_perm(HashT& target) {
        int lo = 0;
        int hi = buff->len / HASH_SIZE;
        HashT tmp;

        while (lo < hi) {
            const size_t mid = lo + (hi - lo) / 2;

            memcpy(&tmp.b, buff->b + mid, HASH_SIZE);
            const int r = memcmp(tmp.b, target.b, HASH_SIZE);
            if (r < 0) {
                lo = mid + 1;
            } else if (r > 0) {
                hi = mid - 1;
            } else {

                unsigned char* cur = buff->b + mid;
                memmove(cur, cur + HASH_SIZE, buff->len - mid - HASH_SIZE);
                return true;
            }
        }
        return false;
    }
};


struct FileHandle {
    HashT        hash;
    FILE*       f;
    uint64_t    size;
    uint8_t     ref_count = 0;
};
struct HashFileHash {
    size_t operator()(const HashT& arr) const noexcept {
        size_t h;
        memcpy(&h, arr.b, sizeof(size_t));
        return h;
    }
};

static constexpr size_t SID_SZ = 16;
using SessionID = std::array<unsigned char, SID_SZ>;
struct Session {
    SessionID   id;
    bool        is_inbound;
    FileHandle* file;
    Hasher      hasher;
    int         chunk_idx = 0;
    uint64_t    chunk_size;
    Voucher     voucher;
    uint8_t     actor;

};
struct HashSessionID {
    size_t operator()(const SessionID& arr) const noexcept {
        size_t h;
        memcpy(&h, arr.data(), sizeof(size_t));
        return h;
    }
};

