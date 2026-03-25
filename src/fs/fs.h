#pragma once
#include "p2p/msgs.h"
#include "utils/keys.h"
#include "fs/db.h"
#include "fs/types.h"
#include "msg.h"
#include <deque>
#include <set>
#include <unordered_map>
#include <vector>

namespace fs {

class Manager {
private:
    int                     epoll_fd_;
    msg::SPSCQueue&         from_main_;
    msg::SPSCQueue&         too_main_;
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
        return "";
    }

    bool handle_err(net_msg::Error  e) {
        msg::Msg* m = msgs_.back();
        msgs_.pop_back();
        net_msg::msg_error(m, e, Actors::FILESYS);
        if (!too_main_.push(m)) {
            m->wipe();
            msgs_.push_back(m);
            return false;
        }
        return true;
    }

public:
    Manager(msg::ChannelPair& chan, const char* path, size_t map_size);
    void poll_loop();
};

}
