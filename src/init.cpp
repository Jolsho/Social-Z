#include <cassert>
#include "http/server.h"
#include "p2p/p2p.h"
#include "fs/fs.h"

std::thread start_p2p(
    int main_epoll_fd, 
    ActorChannels& chans,
    P2PConfig config
) {
    register_queue(main_epoll_fd, chans.from);
    std::thread p2p_thread([&]{

        p2p::Manager p2p_mgr(chans, config);
        assert(p2p_mgr.start_server(config) == 0);
        p2p_mgr.poll_loop(); 
    });
    return p2p_thread;
}

std::thread start_fs(
    int main_epoll_fd, 
    ActorChannels& chans,
    FSConfig config
) {
    register_queue(main_epoll_fd, chans.from);
    std::thread filesys_thread([&]{
        fs::Manager file_mgr(chans, config);
        file_mgr.poll_loop(); 
    });
    return filesys_thread;
}

std::thread start_rpc(
    int main_epoll_fd, 
    ActorChannels& chans,
    RPCConfig config
) {
    register_queue(main_epoll_fd, chans.from);
    std::thread rpc_thread([&]{

        Server http_server(chans, config);
        assert(http_server.start_server(config) == 0);
        http_server.poll_loop(); 
    });
    return rpc_thread;
}

