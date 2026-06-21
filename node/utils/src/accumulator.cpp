#include <cstring>
#include <ctime>
#include "utils/accumulator.h"
#include "utils/vec.h"

LogAccumulator::LogAccumulator(
    std::string parent_str, 
    time_t flush_interval, 
    BufferStore &buffs,
    Actors from
) : 
    parent_str_(parent_str),
    flush_interval_(flush_interval),
    buffers_(buffs)
{
    flush_time_ = time(nullptr) + flush_interval_;
    full_.reserve(8);
    l_.data = buffs.grab(BufferSize::XXS);
    l_.too = ACTOR_LOG;
    l_.from = from;
};

size_t LogAccumulator::flush(MsgBuffer* m) {
    time_t now = time(nullptr);
    size_t flushed { 0 };
    if (now >= flush_time_) {
        flush_time_ = now + flush_interval_;
        while (!full_.empty()) {
            Msg* msg = consume_msg(m);
            if (!msg) break;
            msg->priority = PRIORITY_TELE;
            *msg = full_.back();
            full_.pop_back();
            ++flushed;
        }
    }
    return flushed;
}


void LogAccumulator::log(std::string msg, int r, int code) {
    size_t log_size = FIXED_LOG_PART + msg.size();

    if (l_.data->len + log_size >= l_.data->cap) {
        full_.push_back(l_);
        l_.data = buffers_.grab(BufferSize::M);
    }

    std::time_t now = std::time(nullptr);
    char time_buff[20];

    size_t len = std::strftime(
        time_buff, sizeof(time_buff),
        "%Y-%m-%d %H:%M:%S",
        std::localtime(&now)
    );

    vec_write(l_.data, '[');
    vec_write(l_.data, time_buff);
    vec_write(l_.data, ']');

    vec_write(l_.data, ' ');

    vec_write(l_.data, '[');
    vec_write_str(l_.data, parent_str_.c_str());
    vec_write(l_.data, ']');

    vec_write(l_.data, ' ');

    vec_write(l_.data, '"');
    vec_write_str(l_.data, msg.data());
    vec_write(l_.data, '"');

    vec_write(l_.data, '\n');
}

void LogAccumulator::log(ChanStatsPair* stats) {
}
