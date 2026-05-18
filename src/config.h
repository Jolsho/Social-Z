#pragma once
#include <functional>
#include <thread>
#include "chans.h"
#include "codes.h"
#include "crypto.h"

struct P2PConfig {
    size_t      msgs_cap    { 256 };
    size_t      pkts_cap    { 256 };
    uint16_t    port        { 3213 };
    const char* ip          { "0.0.0.0" };
    const char* key_path    { "p2p.key" };
    Key         key         { ZERO_KEY };
};
std::thread start_p2p(
    int main_epoll_fd, 
    ActorChannels& chans,
    P2PConfig config = {}
);


struct FSConfig {
    size_t      msgs_cap    { 256 };
    const char* db_path     { "./db" };
    size_t      map_size    { 10 * 1024 * 1024 };
};
std::thread start_fs(
    int main_epoll_fd, 
    ActorChannels& chans,
    FSConfig config = {}
);

struct RPCConfig {
    size_t      msgs_cap    { 256 };
    uint16_t    port        { 443 };
    const char* ip          { "0.0.0.0" };
    const char* key_path    { "http.key" };
    const char* cert_path   { "http.cert" };
};
std::thread start_rpc(
    int main_epoll_fd, 
    ActorChannels& chans,
    RPCConfig config = {}
);

struct LogConfig {
    size_t      msgs_cap    { 32 };
    const char* path        { "./logs" };
};
std::thread start_logger(
    int main_epoll_fd, 
    ActorChannels& chans,
    LogConfig config = {}
);


template<size_t S>
std::thread main_loop(
    int main_epoll_fd, 
    std::array<ActorChannels, S>& actors,
    std::function<void (Msg*)> callback = nullptr
) {
    const size_t MAX_EVENTS = 64;
    while (true) {
        epoll_event events[MAX_EVENTS];
        int n = epoll_wait(main_epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            // DISPATCH MESSAGES AMONG ACTORS
            for (auto &actor: actors) {
                if (fd == actor.from.get_event_fd()) {
                    actor.from.clear_event();
                    while (Msg* msg = actor.from.pop()) {
                        if (callback) callback(msg);

                        if (msg->too < S) {
                            if (!actors[msg->too].to.push(msg)) {
                                // TODO
                            }
                        } else if (msg->from < Actors::COUNT) {
                            msg_wipe(msg);
                            if (!actors[msg->from].to.push(msg)) {
                                delete msg;
                            }

                        } else {
                            if (msg->code == Code::SHUTDOWN) {
                                // TODO just span msg actors with shutdown
                            }
                            delete msg;
                        }
                    }
                }
            }
            // ANY OTHER THINGS?
        }
    }
}
