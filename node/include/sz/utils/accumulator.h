#pragma once
#include "sz/utils/buffers.h"
#include "sz/api/msgT.h"
#include "sz/api/actor.h"
#include <string>

class LogAccumulator {
    std::string                 parent_str_;
    time_t                      flush_time_;
    time_t                      flush_interval_;
    std::vector<Msg>            full_;
    Msg                         l_;
    BufferStore&                buffers_;

    static constexpr size_t FIXED_LOG_PART  = 30 + sizeof(uint16_t);
    static constexpr size_t PARENT_SIZE     = 6;

public:

    LogAccumulator(
        std::string parent_str, 
        time_t flush_interval, 
        BufferStore& buffers,
        Actors from
    );

    size_t flush(MsgBuffer* m);

    void log(std::string msg, int r = 0, int code = 0);
    void log(ChanStatsPair* stats);
};

