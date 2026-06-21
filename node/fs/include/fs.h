#pragma once
#include "config.h"
#include "sodium/utils.h"
#include "utils/buffers.h"
#include "db_iface.h"
#include "fs_types.h"
#include "utils/accumulator.h"
#include <cstdio>
#include <deque>
#include <set>
#include <unistd.h>
#include <unordered_map>
#include "utils/error.h"
#include "utils/time.h"

namespace fs {

enum class FSCODE: uint16_t {
    VOUCHER = 1,
    REDEEM  = 2,
    REWARD  = 3,
    GIVE    = 4,
    SETTLE  = 5,
    ASK     = 6,
    REVOKE  = 7,
};


class Manager {
private:


    const std::string       fs_root_;
    static constexpr size_t PATH_PARTS = 4;
    std::string             path_;

    LogAccumulator*     logr_;

    int                 epoll_fd_;
    Actor*              chans_;
    BufferStore         buffers_;

    std::set<Key>       locals_;
    LMDB                db_;

    std::unordered_map<HashT, FileHandle, HashFileHash>         open_files_;
    std::unordered_map<SessionID, Session, HashSessionID>       sessions_;

    std::deque<Session> outbound_;

    std::deque<PendingPermBucket>  pending_perms_;

    MsgBuffer*  free_out_msgs_;
    MsgBuffer*  in_msgs_;

    void voucher(const Msg* msg, Error& e);
    void redeem(const Msg* msg, Error& e);
    void reward(const Msg* msg, Error& e);

    void give(const Msg* msg, Error& e);
    void local_give(const Msg* msg, Error& e);

    void settle(const Msg* msg, Error& e);
    void local_settle(const Msg* msg, Error& e);

    void ask(const Msg* msg, Error& e);
    void local_ask(const Msg* msg, Error& e);

    void revoke(const Msg* msg, Error& e);
    void local_revoke(const Msg* msg, Error& e);

    std::string& derive_path(HashT &file_hash) {
        path_.resize(fs_root_.size());

        const size_t b64_len = sodium_base64_encoded_len(HASH_SIZE, sodium_base64_VARIANT_ORIGINAL);
        char hash[b64_len];
        sodium_bin2base64(hash, b64_len, file_hash.b, HASH_SIZE, sodium_base64_VARIANT_ORIGINAL);

        const size_t p_size {b64_len/PATH_PARTS};
        for (int i {0}; i < PATH_PARTS; i++) {
            path_.append(hash + (i * p_size), p_size);
            path_.push_back('/');
        }
        return path_;
    }

    void shutdown() {
        for (auto& [i, s]: sessions_) {
            if (--s.file->ref_count == 0) fclose(s.file->f);

            if (s.is_inbound && s.file->size > (s.chunk_size * s.chunk_idx)) {
                remove(derive_path(s.file->hash).data());
            }
        }

        for (auto& s: outbound_) {
            if (--s.file->ref_count == 0) fclose(s.file->f);

            if (s.is_inbound && s.file->size > (s.chunk_size * s.chunk_idx)) {
                remove(derive_path(s.file->hash).data());
            }
        }

        sessions_.clear();
        outbound_.clear();
        open_files_.clear();


        close(epoll_fd_);
        logr_->log("File System Shutdown Successful.");
        logr_->flush(this->free_out_msgs_);
    }

    bool handle_err(Error  e, Actors too) {
        Msg* m = consume_msg(free_out_msgs_);
        if (!m) return false;
        m->priority = PRIORITY_CRIT;
        marshal_error(e, m, too, [&](size_t s){ return buffers_.grab(s); });
        return true;
    }

    void new_pending_perm(HashT& h) {
        time_t tomo = next_midnight();
        if (pending_perms_.back().expires == tomo) {
            pending_perms_.back().put_next(h);
        } else {
            auto& pps = pending_perms_.emplace_back();
            pps.expires = tomo;
            pps.buff = buffers_.grab(BufferSize::L);
            pps.put_next(h);
        }
    }

public:
    Manager(Actor* chans, FSConfig& conf);
    int initialize();
    void poll_loop();
};

}
