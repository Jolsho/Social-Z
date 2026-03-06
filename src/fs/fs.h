#pragma once
#include "msging.h"

namespace fs {

class FileManager {
    int epoll_fd_;
    msging::SPSCQueue& incoming_queue_;
    msging::SPSCQueue& outgoing_queue_;

public:
    FileManager(msging::ChannelPair& chan);
    void poll_loop();
};

}
