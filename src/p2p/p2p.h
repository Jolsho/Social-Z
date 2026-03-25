#pragma once
#include "msg.h"
#include "p2p/connection.h"
#include "p2p/citizens.h"
#include <unordered_map>

namespace p2p {

static constexpr size_t MAX_CONNECTIONS     = 32;
static constexpr size_t CONNECTION_TIMEOUT  = 20;
static constexpr size_t REVEAL_KEY_TIMEOUT  = 5;

class Manager {
public:

    // MSGING
    int                         epoll_fd_;
    msg::SPSCQueue&             from_main_;
    msg::SPSCQueue&             to_main_;
    std::vector<msg::Msg*>      msgs_;

    // TCP SERVER
    int                                 listen_fd_;
    KeyPair                             keys_;
    std::vector<Packet*>                pkts_;
    std::vector<conn::Connection>       connections_;
    std::vector<time_t> expirations_;

    ConnID                              next_id_;
    std::vector<ConnID>                 free_ids_;
    std::unordered_map<int, ConnID>     sock_ids_;

    std::unordered_map<Key, Citizen, KeyHash> citizens_;

    Manager(msg::ChannelPair& chan, size_t msgs_cap, size_t pkts_cap) : 
        from_main_(chan.to), 
        to_main_(chan.from),
        msgs_(msgs_cap),
        pkts_(pkts_cap),
        expirations_(MAX_CONNECTIONS)
    {
        connections_.reserve(MAX_CONNECTIONS);
        free_ids_.reserve(MAX_CONNECTIONS);
        sock_ids_.reserve(MAX_CONNECTIONS);
    }

    void poll_loop();
    int start_server(const char* ip, ConnID port);
    void remove_socket(ConnID id);
    ConnID add_socket(int sock_fd, const Key pubkey, bool is_inbound);

    Packet* get_pkt() {
        Packet* pkt;
        if (pkts_.size() > 0) {
            pkt = pkts_.back();
            pkts_.pop_back();
        } else {
            pkt = new Packet;
        }
        return pkt;
    }

    msg::Msg* get_msg() {
        msg::Msg* msg;
        if (msgs_.size() > 0) {
            msg = msgs_.back();
            msgs_.pop_back();
        } else {
            msg = new msg::Msg;
        }
        return msg;
    }
};
};
