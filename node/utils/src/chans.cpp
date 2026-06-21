#include "utils/chans.h"
#include <cstddef>

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

    size_t free_slots = remaining_space(msgs);

    size_t prev = free_slots;
    size_t laps = 0;

    size_t* c = in_use_.front();
    while (laps < PRIORITY_COUNT && 0 < free_slots) {

        size_t load = std::min(budgets_[current_], free_slots);

        free_slots -= q_[current_].take_elements(in_use_, load);

        for (;free_slots < prev && c != NULL; prev--) {
            *(msgs->msgs_ + msgs->head_) = &msgs_[*c];
            msgs->head_ = (msgs->head_ + 1) % msgs->cap_;
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
    size_t* c = free_.back();
    size_t remaining = remaining_space(msgs);

    while (0 < remaining && c != nullptr) {
        *(msgs->msgs_ + msgs->head_) = &msgs_[*c];
        c = free_.step(c);
        msgs->head_ = (msgs->head_ + 1) % msgs->cap_;
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
