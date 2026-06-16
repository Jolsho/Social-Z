#include "utils/chans.h"

QueueStats ChanSet::stats(Priority p) {
    auto i = idx(p);

    QueueStats s{
        .accepted = accepted_[i],
        .dropped = dropped_[i],
        .delta_accepted = accepted_[i] - accepted_last_log_[i],
        .delta_dropped = dropped_[i] - dropped_last_log_[i]
    };
    s.drop_rate = static_cast<float>(s.delta_accepted + s.delta_dropped) / s.delta_dropped;

    accepted_last_log_[i] = accepted_[i];
    dropped_last_log_[i] = dropped_[i];

    return s;
}


bool ChanSet::front(Msg* m) {
    for (size_t i = 0; i < idx(Priority::Count); ++i) {
        auto* msg = qs_[current_].front();
        if (msg) {
            *m = *msg;
            return true;
        }
        counts_[current_] = 0;
        current_ = (++current_) % idx(Priority::Count);
    }
    return false;
}

void ChanSet::pop(size_t p) {
    if (p != current_) {
        qs_[p].pop();
        return;
    }

    qs_[current_].pop();
    if (++counts_[current_] == budgets_[current_]) {
        counts_[current_] = 0;
        current_ = (++current_) % idx(Priority::Count);
    }
}
