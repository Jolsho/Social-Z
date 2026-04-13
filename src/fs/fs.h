#pragma once
#include "init.h"
#include "fs/db.h"
#include "fs/types.h"
#include "crypto.h"
#include "log/accumulator.h"
#include "msg.h"
#include <deque>
#include <set>
#include <unordered_map>
#include <vector>

namespace fs {

class Manager {
private:
    LogAccumulator*             logr_;

    int                     epoll_fd_;
    MsgChan&                from_main_;
    MsgChan&                too_main_;
    std::vector<msg::Msg*>  msgs_;

    std::set<Key>   locals_;
    DB              db_;

    std::unordered_map<Hash, FileHandle, HashFileHash>      open_files_;
    std::unordered_map<SessionID, Session, HashSessionID>   sessions_;

    std::deque<Session> outbound_;

    void voucher(msg::Msg* msg);
    void redeem(msg::Msg* msg);
    void reward(msg::Msg* msg);

    void give(msg::Msg* msg);
    void accept(msg::Msg* msg);

    void ask(msg::Msg* msg);
    void revoke(msg::Msg* msg);

    std::string derive_path(Hash &file_hash) {
        // TODO
        return {};
    }

    bool handle_err(msg::Error  e) {
        msg::Msg* m = msgs_.back();
        msgs_.pop_back();
        msg::p2p_error(m, e, Actors::FILESYS);
        if (!too_main_.push(m)) {
            m->wipe();
            msgs_.push_back(m);
            return false;
        }
        return true;
    }

    msg::Msg* get_msg() {
        msg::Msg* msg;
        if (msgs_.size() > 0) {
            msg = msgs_.back();
            msgs_.pop_back();
        } else {
            msg = new msg::Msg { Actors::FILESYS };
        }
        msg->is_wiped = false;
        return msg;
    }

public:
    Manager(ActorChannels& chan, FSConfig& conf);
    int initialize();
    void poll_loop();
};

}
