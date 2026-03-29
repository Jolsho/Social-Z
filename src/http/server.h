#pragma once
#include "msg.h"
#include "http/conns.h"
#include "p2p/msgs.h"
#include <set>
#include <unordered_map>

struct Destination {
    std::string path;
    std::string method;
    CODE        code;
    Actors      to;
};

const std::initializer_list<Destination> PATHS = {
    Destination {
        .path = "/",
        .method = "GET",
        .code = CODE::INDEX,
        .to = Actors::FILESYS,
    },

    Destination {
        .path = "/chats",
        .method = "GET",
        .code = CODE::CHATS,
        .to = Actors::SOCIALIZER,
    },
};



static constexpr size_t MAX_CONNECTIONS = 32;

using Token = std::array<unsigned char, 16>;
static constexpr size_t TOKEN_SIZE = sizeof(Token);

class Server {

    // MSGING
    int                         epoll_fd_;
    msg::SPSCQueue&             from_main_;
    msg::SPSCQueue&             to_main_;
    std::vector<msg::Msg*>      msgs_;

    int                         listen_fd_;
    SSL_CTX*                    ctx_;
    llhttp_settings_t           settings_;

    ConnID                              next_id_;
    std::vector<ConnID>                 free_ids_;
    std::unordered_map<int, ConnID>     conn_ids_;

    std::vector<conn_t>                 conns_;
    std::set<Token>                     tokens_;

    std::set<std::string>   banned_ips_;


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

public:

    Server(msg::ChannelPair& chan, size_t msgs_cap, size_t pkts_cap);
    void poll_loop();
    int start_server(
        const char* ip, ConnID port, 
        const char* cert, const char* key
    );
    void handle_error(net_msg::Error e);
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

