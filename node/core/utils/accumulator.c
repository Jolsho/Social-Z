#include "sz/utils/accumulator.h"
#include "sz/utils/vec.h"
#include <string.h>
#include <stdlib.h>

LogAccumulator* new_accumulator(
    const char* parent_str, 
    time_t flush_interval, 
    BufferStore *buffs,
    Actors from
) {
    LogAccumulator* la = (LogAccumulator*)malloc(sizeof(LogAccumulator));
    la->parent_str_ = parent_str;
    la->flush_interval_ = flush_interval;
    la->buffers_ = buffs;
    la->flush_time_ = time(NULL) + la->flush_interval_;
    la->full_cap_ = 8;
    la->full_size_ = 0;
    la->full_ = (Msg*)malloc(sizeof(Msg) * la->full_size_);
    la->l_.data = grab_buff(buffs, BUFF_XXS);
    la->l_.too = ACTOR_LOG;
    la->l_.from = from;

    return la;
};

size_t flush(LogAccumulator* l, MsgBuffer* m) {
    time_t now = time(NULL);
    size_t flushed  = 0;
    if (now >= l->flush_time_) {
        l->flush_time_ = now + l->flush_interval_;
        while (0 < l->full_size_) {
            Msg* msg = consume_msg(m);
            if (!msg) break;
            msg->priority = PRIORITY_TELE;
            *msg = *(l->full_ + l->full_size_);
            ++flushed;
        }
    }
    return flushed;
}


void log_msg(LogAccumulator* l, const char* msg, int r, int code) {
    size_t log_size = FIXED_LOG_PART + strlen(msg);

    if (l->l_.data->len + log_size >= l->l_.data->cap && l->full_size_ < l->full_cap_) {
        Msg* m = (l->full_ + (++l->full_size_));
        *m = l->l_;
        l->l_.data = grab_buff(l->buffers_, BUFF_M);
    }

    time_t now = time(NULL);
    static size_t TIME_BUFF_SIZE = 20;
    char time_buff[TIME_BUFF_SIZE];

    size_t len = strftime(
        time_buff, sizeof(time_buff),
        "%Y-%m-%d %H:%M:%S",
        localtime(&now)
    );
    static uint8_t close_brack = '[';
    static uint8_t open_brack = ']';
    static uint8_t space = ' ';
    static uint8_t quote = '"';
    static uint8_t esc = '\n';

    vec_write(l->l_.data, &open_brack, 1);
    vec_write(l->l_.data, &time_buff, TIME_BUFF_SIZE);
    vec_write(l->l_.data, &close_brack, 1);

    vec_write(l->l_.data, &space, 1);

    vec_write(l->l_.data, &open_brack, 1);
    vec_write(l->l_.data, l->parent_str_, strlen(l->parent_str_));
    vec_write(l->l_.data, &close_brack, 1);

    vec_write(l->l_.data, &space, 1);

    vec_write(l->l_.data, &quote, 1);
    vec_write(l->l_.data, msg, strlen(msg));
    vec_write(l->l_.data, &quote, 1);

    vec_write(l->l_.data, &esc, 1);
}

void log_stats(LogAccumulator* l, ChanStatsPair* stats) {
    // TODO

}
