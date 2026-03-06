#include "social.h"
#include <cstdio>
#include <sys/epoll.h>

socializer::Socializer::Socializer(msging::ChannelPair& chan)
    : incoming_queue_(chan.to), outgoing_queue_(chan.from) 
{
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) perror("epoll_create1");
}


void socializer::Socializer::poll_loop() {
    const int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];
    while (true) {
        // short timeout
        int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, 1); 

        for (int i = 0; i < n; ++i) {
            incoming_queue_.clear_event();
            while (auto* pkt = incoming_queue_.pop()) {

            }
        }
    }
}
