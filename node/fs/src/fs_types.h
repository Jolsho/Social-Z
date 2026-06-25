#pragma once
#include "fs/fs.h"
#include "hash.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <ctime>



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
    size_t operator()(const HashT& has) const noexcept {
        std::size_t h = 0;
        for (int i = 0; i < HASH_SIZE; i++) {
            h = h * 31 + reinterpret_cast<uint8_t>(has.b[i]);
        }
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

