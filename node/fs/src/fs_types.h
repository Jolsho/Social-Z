#pragma once
#include "fs/fs.h"
#include "hash.h"
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
    SessionID   id;
    bool        is_inbound;
    FileHandle* file;
    Hasher      hasher;
    uint64_t    byte_count;
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

