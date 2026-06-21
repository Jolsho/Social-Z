#pragma once
#include "api/actor.h"
#include "config.h"
#include <cstdio>
#include <string>

class Logger {
    int             epoll_fd_;
    Actor*          chans_;

    MsgBuffer*  free_out_msgs_;
    MsgBuffer*  in_msgs_;

    int     f_;
    int     shutdown_signals_ = 0;

    bool parse_n_write_log(Msg* l);

    std::string derive_file_name();

public:
    Logger(Actor* chan, LogConfig& conf);
    int initialize();
    void poll_loop();
    void shutdown();
};
