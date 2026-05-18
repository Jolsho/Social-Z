#pragma once
#include "log/accumulator.h"
#include "p2p/connection.h"
#include "p2p/citizens.h"
#include "utils/lru.h"
#include <fcntl.h>
#include <functional>
#include <unordered_map>
#include "config.h"

namespace p2p {

static constexpr size_t MAX_CONNECTIONS     = 32;
static constexpr size_t CONNECTION_TIMEOUT  = 20;
static constexpr size_t REVEAL_KEY_TIMEOUT  = 5;

class Manager {
public:
    LogAccumulator*             logr_;

    // MSGING
    int                         epoll_fd_;
    MsgChan&                    from_main_;
    MsgChan&                    to_main_;
    std::vector<Msg*>      msgs_;

    // TCP SERVER
    int                                 listen_fd_;
    KeyPair                             keys_;
    std::vector<Packet*>                pkts_;
    std::vector<conn::Connection>       connections_;
    std::unordered_map<Key, ConnID, KeyHash> key_to_conn_;

    ConnLRU<MAX_CONNECTIONS, CONNECTION_TIMEOUT>   lru_;
    // TODO -- there is an issue here where we dont diffentiate timeouts
    // between live connections and negotiating ones.
    // so incoming connections failing to reveal their key still get the full time
    // they should have much less time than negotiated connections.

    ConnID                              next_id_;
    std::vector<ConnID>                 free_ids_;
    std::unordered_map<int, ConnID>     sock_ids_;

    std::unordered_map<Key, Citizen, KeyHash> citizens_;

    Manager(
        ActorChannels& chan, 
        P2PConfig& conf
    ) : 
        from_main_(chan.to), 
        to_main_(chan.from),
        pkts_(conf.pkts_cap)
    {
        for (int i{0}; i < conf.msgs_cap; i++) {
            Msg* m = new Msg{};
            msg_init(m, Actors::PEERNET, MAX_BUFFER_SIZE);
            msgs_.push_back(m);
        }
        connections_.reserve(MAX_CONNECTIONS);
        free_ids_.reserve(MAX_CONNECTIONS);
        sock_ids_.reserve(MAX_CONNECTIONS);

        static constexpr time_t LOG_FLUSH_INTERVAL = 500; // ms
        logr_ = new LogAccumulator{"P2P", LOG_FLUSH_INTERVAL, [&](){ return get_msg(); }};
    }

    void poll_loop();
    int start_server(P2PConfig &config);


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

    Msg* get_msg() {
        Msg* msg;
        if (msgs_.size() > 0) {
            msg = msgs_.back();
            msgs_.pop_back();
        } else {
            msg = new Msg{};
            msg_init(msg, Actors::PEERNET, MAX_BUFFER_SIZE);
        }
        msg->is_wiped = false;
        return msg;
    }
};
};
