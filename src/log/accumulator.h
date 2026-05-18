#pragma once
#include "chans.h"
#include <functional>
#include <string>

class LogAccumulator {
    std::string                 parent_str_;
    time_t                      flush_time_;
    time_t                      flush_interval_;
    std::vector<Msg*>           full_;
    Msg*                        l_;
    std::function<Msg*()>       get_new_msg_;

    static constexpr size_t FIXED_LOG_PART  = 30 + sizeof(uint16_t);
    static constexpr size_t PARENT_SIZE     = 6;

public:

    LogAccumulator(
        std::string parent_str, 
        time_t flush_interval, 
        std::function<Msg*()> get_new_msg
    );

    size_t flush(MsgChan& out);

    void log(std::string msg);
};

