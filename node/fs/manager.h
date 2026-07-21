/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include <queue>
#include <sodium/utils.h>
#include "sz/utils/key.hpp"
#include "sz/utils/buffers.h"
#include "sz/fs.h"
#include "sz/lmdb.h"
#include "fs_types.h"
#include "sz/utils/accumulator.h"
#include <cstdio>
#include <deque>
#include <set>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include "sz/utils/error.h"
#include "sz/utils/time.h"

class FS {
private:

    size_t                      total_fs_size;
    size_t                      allotted_space;

    const std::string           fs_root_;
    static constexpr size_t     PATH_PARTS = 4;
    std::string                 path_;
    std::string                 tmp_path_;
    LogAccumulator*             logr_;

    Actor*                      chans_;
    BufferStore*                buffers_;

    std::set<Key, KeyCompare>   locals_;
    LMDB*                       db_;

    std::unordered_map<HashT, FileHandle, HashFileHash>     open_files_;
    std::unordered_map<SessionID, Session, HashSessionID>   sessions_;
    std::priority_queue<std::tuple<time_t, SessionID>>       session_expires_;
    std::deque<Session>                                     outbound_;

    std::deque<PendingPermBucket>  pending_perms_;

    MsgBuffer*  free_out_msgs_;
    MsgBuffer*  in_msgs_;

    void voucher        (const Msg* msg, Error& e);
    void redeem_remote  (const Msg* msg, Error& e);
    void reward         (const Msg* msg, Error& e);
    void give_remote    (const Msg* msg, Error& e);
    void settle_remote  (const Msg* msg, Error& e);
    void ask_remote     (const Msg* msg, Error& e);
    void revoke_remote  (const Msg* msg, Error& e);

    void redeem_local   (const Msg* msg, Error& e);
    void give_local     (const Msg* msg, Error& e);
    void settle_local   (const Msg* msg, Error& e);
    void ask_local      (const Msg* msg, Error& e);
    void revoke_local   (const Msg* msg, Error& e);

    std::string tmp_path(SessionID& id) {
        std::string tmp(fs_root_);
        tmp.append("tmp/");
        const size_t b64_len = sodium_base64_encoded_len(SID_SZ, sodium_base64_VARIANT_ORIGINAL);
        char id_str[b64_len];
        sodium_bin2base64(id_str, b64_len, id.data(), HASH_SIZE, sodium_base64_VARIANT_ORIGINAL);
        tmp.append(id_str);
        return tmp;
    }

    std::string& derive_path(HashT &file_hash) {
        path_.resize(fs_root_.size());

        const size_t b64_len = sodium_base64_encoded_len(HASH_SIZE, sodium_base64_VARIANT_ORIGINAL);
        char hash[b64_len];
        sodium_bin2base64(hash, b64_len, file_hash.b, HASH_SIZE, sodium_base64_VARIANT_ORIGINAL);

        const size_t p_size {b64_len/PATH_PARTS};
        for (int i {0}; i < PATH_PARTS; i++) {
            path_.push_back('/');
            path_.append(hash + (i * p_size), p_size);
        }
        return path_;
    }

    void shutdown() {
        for (auto& [i, s]: sessions_) {
            if (--s.file->ref_count == 0) close(s.file->fd);

            if (s.is_inbound && s.file->size > (s.byte_count)) {
                remove(derive_path(s.file->hash).data());
            }
        }

        for (auto& s: outbound_) {
            if (--s.file->ref_count == 0) close(s.file->fd);

            if (s.is_inbound && s.file->size > s.byte_count) {
                remove(derive_path(s.file->hash).data());
            }
        }

        sessions_.clear();
        outbound_.clear();
        open_files_.clear();


        log_msg(logr_, "File System Shutdown Successful.", -1, -1);
        flush(logr_, this->free_out_msgs_);
    }

    bool handle_err(Error  e, Actors too) {
        Msg* m = consume_msg(free_out_msgs_);
        if (!m) return false;
        m->priority = PRIORITY_CRIT;
        m->data = grab_buff(buffers_, error_size(&e));
        marshal_error(&e, m, too);
        return true;
    }

    void new_pending_perm(HashT& h) {
        time_t tomo = next_midnight();
        if (pending_perms_.back().expires == tomo) {
            pending_perms_.back().hashes.emplace_back(h);
        } else {
            auto& pps = pending_perms_.emplace_back();
            pps.expires = tomo;
            pps.hashes.reserve(256);
            pps.hashes.emplace_back(h);
        }
    }
    
    void handle_outbound();
    void handle_internal_msgs();

public:
    FS(Actor* chans, FSConfig* conf);
    void poll_loop();
};
