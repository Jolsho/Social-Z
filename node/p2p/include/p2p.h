#pragma once
#include "bindings.h"
#include "utils/accumulator.h"
#include "connection.h"
#include "citizens.h"
#include "messenger.h"
#include "utils/buffers.h"
#include "utils/ids.h"
#include "utils/lru.h"
#include <cstdlib>
#include <fcntl.h>
#include <unordered_map>
#include "config.h"


namespace p2p {

enum class Code : int {
    CloseConn,
    NewConn,
    Broadcast,
    Ping,
    Pong,
};
constexpr int code(Code p) { return static_cast<int>(p); }

static constexpr size_t MAX_CONNECTIONS     = 32;
static constexpr size_t CONNECTION_TIMEOUT  = 20;
static constexpr size_t NEGOTIATION_TIMEOUT = 5;

class Manager : 
    public Ids
{
public:
    LogAccumulator*     logr_;
    Messenger           messenger_;

    // MSGING
    int                 epoll_fd_;
    Actor*              chans_;
    BufferStore         buffers_;

    // TCP SERVER
    int                                             listen_fd_;
    KeyPair                                         keys_;
    std::vector<conn::Connection>                   connections_;
    std::unordered_map<Key, ConnID, KeyHash>        key_to_conn_;
    std::unordered_map<int, ConnID>                 sock_ids_;
    CitizenMap                                      citizens_;

    ConnLRU<MAX_CONNECTIONS, CONNECTION_TIMEOUT>    lru_;

    std::deque<std::pair<ConnID, time_t>>    negotiating_timeouts_;

    MsgBuffer*  out_msgs_;
    MsgBuffer*  in_msgs_;



    Manager(Actor* chan, P2PConfig& conf) : 
        chans_(chan), 
        messenger_(conf.wave, conf.broad_msgs),
        buffers_(BufferCaps{}),
        Ids(MAX_CONNECTIONS)
    {
        connections_.reserve(MAX_CONNECTIONS);
        sock_ids_.reserve(MAX_CONNECTIONS);

        static constexpr time_t LOG_FLUSH_INTERVAL = 500; // ms
        logr_ = new LogAccumulator{"P2P", LOG_FLUSH_INTERVAL, buffers_, ACTOR_P2P};
    }

    void poll_loop();
    int start_server(P2PConfig &config);
    void shutdown();


    // KINDA PRIVATE
    void remove_socket(ConnID id);
    ConnID add_socket(int sock_fd, const Key pubkey, bool is_inbound);
    std::optional<Error> connect(Key& pubkey, ConnID* id);

    Error handle_msg(Msg* msg);
    void handle_error(Error e);
    Error p2p_protocols(conn::Connection& c);

    Error readable_conn(conn::Connection& conn);
    Error writeable_conn(conn::Connection& conn);

};
};
