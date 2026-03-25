#include <array>
#include <cassert>
#include <sodium/crypto_auth.h>
#include <sys/epoll.h>
#include <thread>
#include <sodium.h>
#include <array>

#include "fs/fs.h"
#include "p2p/p2p.h"

int main() {
    assert(sodium_init() != -1); 

    int main_epoll_fd = epoll_create1(0);

    std::array<msg::ChannelPair, Actors::COUNT> actors{
        msg::ChannelPair{    // NETWORKER
            .from   = msg::SPSCQueue{256}, 
            .to     = msg::SPSCQueue{256}
        },
        msg::ChannelPair{    // FILE SYSTEM
            .from   = msg::SPSCQueue{256}, 
            .to     = msg::SPSCQueue{256}
        },
        msg::ChannelPair{    // RPC
            .from   = msg::SPSCQueue{256}, 
            .to     = msg::SPSCQueue{256}
        },
        msg::ChannelPair{    // BLOCKCHAIN
            .from   = msg::SPSCQueue{256}, 
            .to     = msg::SPSCQueue{256}
        },
    };

    auto &net = actors[Actors::PEERNET];
    msg::register_queue(main_epoll_fd, net.from);
    std::thread p2p_thread([&]{

        size_t msgs_cap{ 32 };
        size_t pkts_cap{ 32 };
        uint16_t port{ 8080 };
        const char* ip = "127.0.0.1";

        p2p::Manager p2p_mgr(net, msgs_cap, pkts_cap);
        assert(p2p_mgr.start_server(ip, port) == 0);
        p2p_mgr.poll_loop(); 
    });

    auto &filesys = actors[Actors::FILESYS];
    register_queue(main_epoll_fd, filesys.from);
    std::thread filesys_thread([&]{

        const char* db_path = "./db";
        size_t map_size = 10 * 1024 * 1024;

        fs::Manager file_mgr(filesys, db_path, map_size);
        file_mgr.poll_loop(); 
    });

    while (true) {
        epoll_event events[16];
        int n = epoll_wait(main_epoll_fd, events, 16, -1);

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            // DISPATCH MESSAGES AMONG ACTORS
            for (auto &actor: actors) {
                if (fd == actor.from.get_event_fd()) {
                    actor.from.clear_event();
                    while (auto* pkt = actor.from.pop()) {
                        if (pkt->too < Actors::COUNT) {
                            actors[pkt->too].to.push(pkt);
                        } else if (pkt->too < Actors::COUNT) {
                            pkt->wipe();
                            actors[pkt->from].to.push(pkt);
                        } else {
                            delete pkt;
                        }
                    }
                }
            }



        }
    }

    p2p_thread.join();
    filesys_thread.join();
}
