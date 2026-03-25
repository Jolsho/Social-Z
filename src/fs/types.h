#pragma once
#include "msg.h"
#include "utils/hash.h"
#include <cstdio>
#include <cstring>

struct FileHandle {
    Hash        hash;
    FILE*       f;
    uint64_t    size;
    uint8_t     ref_count = 0;
    bool        should_exist = false;
};
struct HashFileHash {
    size_t operator()(const Hash& arr) const noexcept {
        size_t h;
        memcpy(&h, arr.data(), sizeof(size_t));
        return h;
    }
};

using SessionID = std::array<std::byte, 16>;
static constexpr size_t SID_SZ = sizeof(SessionID);
struct Session {
    SessionID   id;
    bool        is_inbound;
    FileHandle* file;
    Hasher      hasher;
    int         chunk_idx = 0;
    uint64_t    chunk_size = msg::MAX_BUFFER_SIZE - SID_SZ;
};
struct HashSessionID {
    size_t operator()(const SessionID& arr) const noexcept {
        size_t h;
        memcpy(&h, arr.data(), sizeof(size_t));
        return h;
    }
};
