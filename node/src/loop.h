#include <functional>
#include <thread>
#include "utils/chans.h"

template<size_t S>
std::thread main_loop(
    int main_epoll_fd, 
    std::array<ActorChannel, S>& actors,
    std::function<void (Msg*)> callback = nullptr
);
