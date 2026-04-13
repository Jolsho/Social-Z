#pragma once
#include "chans.h"
#include "init.h"
#include <cstdio>

class Logger {
    int                     epoll_fd_;
    MsgChan&                from_main_;
    MsgChan&                to_main_;
    std::vector<msg::Msg*>  msgs_;

    int                     f_;

    bool write_log(msg::Msg* l);

    std::string derive_file_name();

public:
    Logger(ActorChannels& chan, LogConfig& conf);
    int initialize();
    void poll_loop();
};
