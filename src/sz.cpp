#include <cassert>
#include "http/server.h"
#include "p2p/p2p.h"
#include "fs/fs.h"

#include <sodium.h>

struct SZ {
    std::array<ActorChannels, Actors::COUNT> actors_ = {};
    std::array<std::thread, Actors::COUNT>  threads_ = {};

    P2PConfig   p2p_config = {};
    RPCConfig   rpc_config = {};
    FSConfig    fs_config  = {};
};

SZ* start_sz() {
    SZ* sz = new SZ();

    assert(sodium_init() != -1); 
    int main_epoll_fd = epoll_create1(0);

    // P2P
    register_queue(main_epoll_fd, sz->actors_[Actors::PEERNET].from);
    sz->threads_[Actors::PEERNET] = std::thread([&]{
        p2p::Manager p2p_mgr(sz->actors_[Actors::PEERNET], sz->p2p_config);
        assert(p2p_mgr.start_server(sz->p2p_config) == 0);
        p2p_mgr.poll_loop(); 
    });

    // FS
    register_queue(main_epoll_fd, sz->actors_[Actors::FILESYS].from);
    sz->threads_[Actors::FILESYS] = std::thread([&]{
        fs::Manager file_mgr(sz->actors_[Actors::FILESYS], sz->fs_config);
        file_mgr.poll_loop(); 
    });

    // RPC
    std::thread rpc_thread = start_rpc(main_epoll_fd, sz->actors_[Actors::RPC_SERVER]);
    register_queue(main_epoll_fd, sz->actors_[Actors::RPC_SERVER].from);
    sz->threads_[Actors::RPC_SERVER] = std::thread([&]{
        Server http_server(sz->actors_[Actors::RPC_SERVER], sz->rpc_config);
        assert(http_server.start_server(sz->rpc_config) == 0);
        http_server.poll_loop(); 
    });

    // MAIN
    sz->threads_[Actors::COUNT] = main_loop(main_epoll_fd, sz->actors_);

    return 0;
}

void block_stop(SZ* sz) {
    for (auto& t: sz->threads_) {
        t.join();
    }
    // TODO handle errored setup / closure/shutdown
}
