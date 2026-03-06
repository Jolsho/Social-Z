#pragma once
#include "msging.h"

namespace socializer {

class Socializer {
    int epoll_fd_;
    msging::SPSCQueue& incoming_queue_;
    msging::SPSCQueue& outgoing_queue_;

public:
    Socializer(msging::ChannelPair& chan);
    void poll_loop();
};

}
