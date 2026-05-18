#include <cstring>
#include <ctime>
#include "log/accumulator.h"
#include "codes.h"
#include "msg.h"

LogAccumulator::LogAccumulator(
    std::string parent_str, 
    time_t flush_interval, 
    std::function<Msg*()> get_new_msg
) : 
    parent_str_(parent_str),
    get_new_msg_(get_new_msg),
    flush_interval_(flush_interval)
{
    flush_time_ = time(nullptr) + flush_interval_;
    full_.reserve(8);
    l_ = get_new_msg_();
    l_->too = Actors::LOGGER;
    l_->code = Code::LOG;
};

size_t LogAccumulator::flush(MsgChan& out) {
    time_t now = time(nullptr);
    size_t flushed{ 0 };
    if (now >= flush_time_) {
        flush_time_ = now + flush_interval_;
        while (!full_.empty()) {
            Msg* next = full_.back();

            if (!next || !out.push(next)) break;

            full_.pop_back();
            ++flushed;
        }
    }
    return flushed;
}

void LogAccumulator::log(std::string msg) {
    size_t log_size = FIXED_LOG_PART + msg.size();

    if (l_->data_len + log_size >= l_->data_cap) {
        full_.push_back(l_);
        l_ = get_new_msg_();
        l_->too = Actors::LOGGER;
        l_->code = Code::LOG;
    }

    size_t old_size = l_->data_len;
    msg_resize(l_, old_size + log_size);
    char* cursor = reinterpret_cast<char*>(l_->data + old_size);

    *cursor++ = '[';                                    // 1

    std::time_t now = std::time(nullptr);
    char time_buff[20];

    size_t len = std::strftime(
        time_buff, sizeof(time_buff),
        "%Y-%m-%d %H:%M:%S",
        std::localtime(&now)
    );

    memcpy(cursor, time_buff, len);                     // 19
    cursor += len;

    *cursor++ = ']';                                    // 1
    *cursor++ = ' ';                                    // 1


    memcpy(cursor, parent_str_.data(), PARENT_SIZE);    // 6 = PARENT_SIZE
    cursor += parent_str_.size();

    *cursor++ = ' ';                                    // 1

    memcpy(cursor, msg.data(), msg.size());             // VAR
    cursor += msg.size();

    *cursor++ = '\n';                                   // 1
                                                        // --
                                                        //  30 = FIXED_LOG_PART
}
