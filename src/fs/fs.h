#pragma once
#include "fs/db.h"
#include "msg.h"
#include "types.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <set>
#include <unordered_map>
#include <vector>

namespace fs {

using FileHash = std::array<std::byte, 32>;
struct FileHandle {
    FileHash    hash;
    int         fd;
    uint64_t    size;
    uint8_t     ref_count;
};
struct HashFileHash {
    size_t operator()(const FileHash& arr) const noexcept {
        size_t h;
        memcpy(&h, arr.data(), sizeof(size_t));
        return h;
    }
};

using SessionID = std::array<std::byte, 16>;
struct Session {
    bool        is_inbound;
    FileHandle* file;
    int         cursor;
    uint64_t    chunk_size;
};
struct HashSessionID {
    size_t operator()(const SessionID& arr) const noexcept {
        size_t h;
        memcpy(&h, arr.data(), sizeof(size_t));
        return h;
    }
};

class Manager {
    int                     epoll_fd_;
    msg::SPSCQueue&         from_main_;
    msg::SPSCQueue&         too_main_;
    std::vector<msg::Msg*>  msgs_;

    std::set<Key>   locals_;
    DB              db_;

    std::unordered_map<FileHash, FileHandle, HashFileHash> open_files_;
    std::unordered_map<SessionID, Session, HashSessionID> sessions_;

public:
    Manager(msg::ChannelPair& chan, const char* path, size_t map_size);
    void poll_loop();
    void handle_msg(msg::Msg* msg);
};

}

/*
 *  DB
 *      PERMISSIONS
 *          - copies of permission hashes
 *          - validates whether perms are revoked or held
 *      META
 *          - meta data of files
 *          - hashes, size, etc.
 *
 *  OPEN_FILES
 *      - essentially just a file handle cache.
 *
 *  IN_SESSIONS
 *      - random id -> file_id / other data
 *
 *  new_card() 
 *      - opens a IN_SESSION given the person is authorized.
 *      - then sends this session id to the sender.
 *          like a session token.
 *
 *  in_blob() { could be from RPC or NET }
 *      - parses the session token.
 *      - writes to file, updates metadata.
 *          - if error send cancel msg back.
 *      - if last validate contents(hash/signature).
 *
 *  out_blob() { could be from RPC or NET }
 *      - ensures has permissions and fileHash exists locally.
 *          - either via DB or OPEN_FILES
 *      - uses given sessions token to send chunks.
 *      - if file meta data has ONE_TIME == true delete file.
 */
