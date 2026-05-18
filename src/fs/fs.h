#pragma once
#include "config.h"
#include "fs/db.h"
#include "fs/types.h"
#include "crypto.h"
#include "log/accumulator.h"
#include "msg.h"
#include <deque>
#include <set>
#include <unordered_map>
#include <vector>
#include "msg/p2p.h"

namespace fs {

class Manager {
private:
    LogAccumulator*             logr_;

    int                         epoll_fd_;
    MsgChan&                    from_main_;
    MsgChan&                    too_main_;
    std::vector<Msg*>      msgs_;

    std::set<Key>               locals_;
    DB                          db_;

    std::unordered_map<Hash, FileHandle, HashFileHash>      open_files_;
    std::unordered_map<SessionID, Session, HashSessionID>   sessions_;

    std::deque<Session> outbound_;

    void voucher(Msg* msg);
    void redeem(Msg* msg);
    void reward(Msg* msg);

    void give(Msg* msg);
    void accept(Msg* msg);

    void ask(Msg* msg);
    void revoke(Msg* msg);

    std::string derive_path(Hash &file_hash) {
        // TODO
        return {};
    }

    bool handle_err(Error  e) {
        Msg* m = msgs_.back();
        msgs_.pop_back();
        p2p_error(m, e, Actors::FILESYS);
        if (!too_main_.push(m)) {
            msg_wipe(m);
            msgs_.push_back(m);
            return false;
        }
        return true;
    }

    Msg* get_msg() {
        Msg* msg;
        if (msgs_.size() > 0) {
            msg = msgs_.back();
            msgs_.pop_back();
        } else {
            msg = new Msg {};
            msg_init(msg, Actors::FILESYS, MAX_BUFFER_SIZE);
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
