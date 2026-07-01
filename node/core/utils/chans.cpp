#include "sz/utils/chans.h"
#include <cstddef>
#include <cstring>

QueueStats Channel::stats(Priority p) {
    QueueStats s{
        .accepted = accepted_[p],
        .dropped = dropped_[p],
        .delta_accepted = accepted_[p] - accepted_last_log_[p],
        .delta_dropped = dropped_[p] - dropped_last_log_[p]
    };
    s.drop_rate = static_cast<float>(s.delta_accepted + s.delta_dropped) / s.delta_dropped;

    accepted_last_log_[p] = accepted_[p];
    dropped_last_log_[p] = dropped_[p];

    return s;
}


void Channel::poll(MsgBuffer* msgs) {

    memmove(msgs->msgs_, (msgs->msgs_ + msgs->cursor_), msgs->size_ - msgs->cursor_);
    msgs->size_ -= msgs->cursor_;
    msgs->cursor_ = 0;

    size_t free_slots = msgs->cap_ - msgs->size_;

    size_t laps = 0;

    size_t* c = in_use_.front();
    while (laps < PRIORITY_COUNT && 0 < free_slots) {


        size_t loaded = q_[current_].take_elements(
            in_use_, 
            std::min(budgets_[current_], free_slots)
        );
        free_slots -= loaded;

        for (;0 < loaded && c != NULL; loaded--) {
            *(msgs->msgs_ + msgs->size_) = &msgs_[*c];
            msgs->size_++;
            c = in_use_.step(c);
        }

        if (counts_[current_] >= budgets_[current_]) {
            counts_[current_] = 0;
            current_ = ++current_ % PRIORITY_COUNT;
            ++laps;
        }
    }
}

void Channel::free_msgs(size_t size) {
    while (0 < size) {
        size_t idx = *in_use_.front();
        in_use_.pop_front();

        msg_wipe(&msgs_[idx]);

        *free_.reserve() = idx;
        free_.commit();
        size--;
    }
}

void Channel::get_free_msgs(MsgBuffer* msgs) {
    memmove(msgs->msgs_, (msgs->msgs_ + msgs->cursor_), msgs->size_ - msgs->cursor_);
    msgs->size_ -= msgs->cursor_;
    msgs->cursor_ = 0;

    size_t remaining = msgs->cap_ - msgs->size_;

    size_t* c = free_.back();

    while (0 < remaining && c != nullptr) {
        *(msgs->msgs_ + msgs->size_) = &msgs_[*c];
        msgs->size_++;
        c = free_.step(c);
    }
}

void Channel::use_free_msgs(size_t size) {
    for (int i = 0; i < size; i++) {

        size_t idx = *free_.back();
        free_.pop_back();

        Priority p = msgs_[idx].priority;

        size_t* q_idx = q_[p].reserve();
        size_t* u_idx = in_use_.reserve();
        if (!q_idx || !u_idx) {
            dropped_[p]++;
            continue;
        }

        *q_idx = idx;
        q_[p].commit();

        *u_idx = idx;
        in_use_.commit();

        accepted_[p]++;
    }
}
