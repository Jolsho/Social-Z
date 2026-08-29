/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_node/p2p.h"
#include "conn_types.h"
#include "citizens.h"
#include "messenger.h"
#include "ids.hpp"
#include "sz_node/utils/accumulator.h"


static constexpr size_t MAX_CONNECTIONS     = 32;
static constexpr size_t CONNECTION_TIMEOUT  = 20;
static constexpr size_t NEGOTIATION_TIMEOUT = 5;

class P2P : public Ids {
    P2PConfig* conf_;

public:
    LogAccumulator*     logr_;
    Messenger           messenger_;

    // MSGING
    Actor*              chans_;
    BufferStore*        buffers_;

    // TCP SERVER
    int                                             listen_fd_;
    KeyPair                                         keys_;
    std::vector<Connection>                         connections_;
    std::unordered_map<Key, ConnID, KeyHash>        key_to_conn_;
    std::unordered_map<int, ConnID>                 sock_ids_;
    CitizenMap                                      citizens_;

    ConnLRU<MAX_CONNECTIONS, CONNECTION_TIMEOUT>    lru_;

    std::deque<std::pair<ConnID, time_t>>    negotiating_timeouts_;

    MsgBuffer*  free_out_msgs_;
    MsgBuffer*  in_msgs_;



    P2P(Actor* chan, P2PConfig* conf) : 
        chans_(chan), 
        messenger_(conf->wave, conf->broad_msgs),
        Ids(MAX_CONNECTIONS)
    {
        buffers_ = new_buffer_store(NULL);
        connections_.reserve(MAX_CONNECTIONS);
        sock_ids_.reserve(MAX_CONNECTIONS);

        conf_ = new P2PConfig;
        *conf_ = *conf;

        static constexpr time_t LOG_FLUSH_INTERVAL = 500; // ms
        logr_ = new LogAccumulator{"P2P", LOG_FLUSH_INTERVAL, ACTOR_P2P};
    }

    void poll_loop();
    int start_server();
    void shutdown();


    // KINDA PRIVATE
    void remove_socket(ConnID id);
    ConnID add_socket(int sock_fd, const Key pubkey, bool is_inbound);
    std::optional<Error> connect(Key& pubkey, ConnID* id);

    Error handle_msg(Msg* msg);
    void handle_error(Error e);
    Error p2p_protocols(Connection& c);

    Error readable_conn(Connection& conn);
    Error writeable_conn(Connection& conn);

};
