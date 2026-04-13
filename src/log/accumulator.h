#pragma once
#include "chans.h"
#include "msg.h"
#include <functional>

class LogAccumulator {
    std::string                 parent_str_;
    time_t                      flush_time_;
    time_t                      flush_interval_;
    std::vector<msg::Msg*>      full_;
    msg::Msg*                   l_;
    std::function<msg::Msg*()>  get_new_msg_;

    static constexpr size_t FIXED_LOG_PART  = 30 + sizeof(uint16_t);
    static constexpr size_t PARENT_SIZE     = 6;

public:

    LogAccumulator(
        std::string parent_str, 
        time_t flush_interval, 
        std::function<msg::Msg*()> get_new_msg
    );

    size_t flush(MsgChan& out);

    void log(std::string msg);
};

