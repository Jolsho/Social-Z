#include "utils/chans.h"
#include "bindings.h"
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


size_t Channel::poll(std::span<Msg*>&& msgs) {

    size_t i    = 0;
    size_t prev = 0;
    size_t laps = 0;

    size_t* c = in_use_.front();
    while (laps < PRIORITY_COUNT && i < msgs.size()) {

        size_t load = std::min(budgets_[current_], msgs.size() - i);

        i += q_[current_].take_elements(in_use_, load);

        for (;prev < i && c != NULL; prev++) {
            msgs[prev] = &msgs_[*c];
            c = in_use_.step(c);
        }

        if (counts_[current_] >= budgets_[current_]) {
            counts_[current_] = 0;
            current_ = ++current_ % PRIORITY_COUNT;
            ++laps;
        }
    }

    return i;
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

size_t Channel::get_free_msgs(std::span<Msg*>&& msgs) {
    size_t i = 0;
    size_t* c = free_.back();

    while (i < msgs.size() && c != nullptr) {
        msgs[i++] = &msgs_[*c];
        c = free_.step(c);
    }

    return i;
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
