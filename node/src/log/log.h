#pragma once
#include "utils/chans.h"
#include "config.h"
#include <cstdio>
#include <string>

class Logger {
    int             epoll_fd_;
    ActorChannel&    chans_;

    int     f_;
    int     shutdown_signals_ = 0;

    bool parse_n_write_log(Msg* l);

    std::string derive_file_name();

public:
    Logger(ActorChannel& chan, LogConfig& conf);
    int initialize();
    void poll_loop();
    void shutdown();
};
