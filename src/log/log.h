#pragma once
#include "chans.h"
#include "config.h"
#include <cstdio>

class Logger {
    int                     epoll_fd_;
    MsgChan&                from_main_;
    MsgChan&                to_main_;
    std::vector<Msg*>  msgs_;

    int                     f_;

    bool write_log(Msg* l);

    std::string derive_file_name();

public:
    Logger(ActorChannels& chan, LogConfig& conf);
    int initialize();
    void poll_loop();
};
