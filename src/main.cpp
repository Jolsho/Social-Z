#include <cassert>
#include <sodium/crypto_auth.h>
#include <sys/epoll.h>
#include <thread>
#include <sodium.h>
#include <array>

#include "init.h"

int main() {
    assert(sodium_init() != -1); 

    int main_epoll_fd = epoll_create1(0);

    std::array<ActorChannels, Actors::COUNT> actors {};

    std::thread p2p_thread = start_p2p(main_epoll_fd, actors[Actors::PEERNET]);
    std::thread fs_thread = start_fs(main_epoll_fd, actors[Actors::FILESYS]);
    std::thread rpc_thread = start_rpc(main_epoll_fd, actors[Actors::RPC_SERVER]);

    std::thread main = main_loop(main_epoll_fd, actors);

    // TODO handle errored setup / closure/shutdown
    rpc_thread.join();
    p2p_thread.join();
    fs_thread.join();
    main.join();
}
