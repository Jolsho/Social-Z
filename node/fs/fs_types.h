#pragma once
#include "sz/api/actor.h"
#include "sz/hash.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <vector>


struct PendingPermBucket {
    time_t              expires;
    std::vector<HashT>  hashes;
};


struct FileHandle {
    HashT       hash;
    int         fd;
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
    SessionID   id = {0};
    bool        is_inbound = false;
    FileHandle* file = nullptr;
    Hasher      hasher;
    uint64_t    byte_count = 0;
    Voucher     voucher;
    uint8_t     actor = ACTOR_NONE;

};
struct HashSessionID {
    size_t operator()(const SessionID& arr) const noexcept {
        size_t h;
        memcpy(&h, arr.data(), sizeof(size_t));
        return h;
    }
};
