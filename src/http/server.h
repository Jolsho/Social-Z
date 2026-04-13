#pragma once
#include "http/conns.h"
#include "chans.h"
#include "init.h"
#include "log/accumulator.h"
#include "msg.h"
#include "utils/lru.h"
#include <array>
#include <set>
#include <unordered_map>

struct Destination {
    std::string path;
    std::string method;
    Code        code;
    Actors      to;
};

const std::initializer_list<Destination> PATHS = {
    Destination {
        .path   = "/",
        .method = "GET",
        .code   = Code::INDEX,
        .to     = Actors::FILESYS,
    },
    Destination {
        .path   = "/login",
        .method = "POST",
        .code   = Code::LOGIN,
        .to     = Actors::SOCIALIZER,
    },
    Destination {
        .path   = "/chats",
        .method = "GET",
        .code   = Code::GET_CHATS,
        .to     = Actors::SOCIALIZER,
    },
    Destination {
        .path   = "/chats",
        .method = "POST",
        .code   = Code::PUT_CHAT,
        .to     = Actors::SOCIALIZER,
    }
};



static constexpr size_t MAX_CONNECTIONS = 32;
static constexpr time_t CONN_EXPIRATION = 5;

using Token = std::array<unsigned char, 16>;
static constexpr size_t TOKEN_SIZE = sizeof(Token);

class Server {

    LogAccumulator*             logr_;


    // MSGING
    int                         epoll_fd_;
    MsgChan&                    from_main_;
    MsgChan&                    to_main_;
    std::vector<msg::Msg*>      msgs_;

    int                         listen_fd_;
    SSL_CTX*                    ctx_;
    llhttp_settings_t           settings_;

    ConnID                              next_id_;
    std::vector<ConnID>                 free_ids_;
    std::unordered_map<int, ConnID>     conn_ids_;

    ConnLRU<MAX_CONNECTIONS, CONN_EXPIRATION>   lru_;
    std::vector<conn_t>                 conns_;
    std::set<Token>                     tokens_;

    std::set<std::string>   banned_ips_;


    msg::Msg* get_msg() {
        msg::Msg* msg;
        if (msgs_.size() > 0) {
            msg = msgs_.back();
            msgs_.pop_back();
        } else {
            msg = new msg::Msg { Actors::RPC_SERVER };
        }
        msg->is_wiped = false;
        return msg;
    }

public:

    Server(
        ActorChannels& chan, 
        RPCConfig& config
    );
    void poll_loop();
    int start_server(RPCConfig& conf);
    void handle_error(msg::Error e);
    void handle_msg(msg::Msg* m);

    void accept_new_connections(int listen_fd, int epfd, SSL_CTX *ctx);
    void close_connection(conn_t& c);

    int build_n_send_msg(conn_t& c);

};

struct HandlerData {
    conn_t* conn;
    Server* server;
};
using Data = std::pair<Server&, conn_t&>;
inline Data cast_data(llhttp_t* p) {
    return {
        *(static_cast<HandlerData*>(p->data))->server, 
        *(static_cast<HandlerData*>(p->data))->conn
    };
}

