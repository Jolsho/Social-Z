#include "utils/chans.h"
#include <cstdlib>
#include <sys/epoll.h>
#include <sys/eventfd.h>

class Actor {

private:
    time_t next_snapshot = time(nullptr) + 20;
    ChanStatsPair stats_;

public:
    Actors      id_;

    int         out_event_fd_;
    Channel*    out_;
    MsgBuffer   out_msg_view_ = {};

    int         in_event_fd_;
    Channel*    in_;
    MsgBuffer   in_msg_view_ = {};


    ChanStatsPair *poll_telemetry() {
        time_t now = time(nullptr);
        if (now < next_snapshot) {
            next_snapshot = now + 10;

            stats_.timestamp = now;
            stats_.in = in_->snapshot();
            stats_.out = out_->snapshot();
            return &stats_;
        }
        return nullptr;
    }

    inline bool write_out_event() {
        const uint64_t value = 1;
        return write(out_event_fd_, &value, sizeof(uint64_t)) == sizeof(uint64_t);
    }

    inline bool write_in_event() {
        const uint64_t value = 1;
        return write(in_event_fd_, &value, sizeof(uint64_t)) == sizeof(uint64_t);
    }

    inline bool clear_in_event() {
        uint64_t val;
        return read(in_event_fd_, &val, sizeof(uint64_t)) == sizeof(uint64_t);
    }

    inline bool clear_out_event() {
        uint64_t val;
        return read(out_event_fd_, &val, sizeof(uint64_t)) == sizeof(uint64_t);
    }

};

Actor* new_actor(ActorConfig* config) {
    Actor* a = new Actor();
    a->id_ = config->id;
    a->out_event_fd_ = eventfd(0, EFD_NONBLOCK);
    a->in_event_fd_ = eventfd(0, EFD_NONBLOCK);

    if (
        a->out_event_fd_ < 0 || 
        a->in_event_fd_ < 0 
    ) {
        delete a;
        return nullptr;
    }

    ActorConfig& c = *config;

    size_t in_msg_cap = 0;
    size_t out_msg_cap = 0;

    ChanArray<size_t> in_sizes;
    ChanArray<size_t> in_buds;

    ChanArray<size_t> out_sizes;
    ChanArray<size_t> out_buds;
    if (
        c.in_q_sizes_len != in_sizes.size() ||
        c.in_q_sizes_len != c.in_budgets_len ||
        c.in_q_sizes_len != c.out_budgets_len ||
        c.in_q_sizes_len != c.out_q_sizes_len
    ) {
        delete a;
        return nullptr;
    }

    for (int i = 0; i < c.in_budgets_len; i++) {
        in_msg_cap += *c.in_q_sizes;
        in_sizes[i] = *(c.in_q_sizes++);
        in_buds[i] = *(c.in_budgets++);


        out_msg_cap += *c.out_q_sizes;
        in_sizes[i] = *(c.out_q_sizes++);
        out_buds[i] = *(c.out_budgets++);
    }

    a->in_ = new Channel(
        in_msg_cap, 
        a->id_, in_sizes, in_buds
    );

    a->out_ = new Channel(
        out_msg_cap, 
        a->id_, out_sizes, out_buds
    );

    return a;
}

int in_event_fd(Actor* a) { return a->in_event_fd_; }
int out_event_fd(Actor* a) { return a->out_event_fd_; }
int register_actor_output_with_epoll(int epoll_fd, Actor* a) {
    epoll_event ev{.events = EPOLLIN, .data = {.fd = out_event_fd(a)}};
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, out_event_fd(a), &ev);
}
int register_actor_input_with_epoll(int epoll_fd, Actor* a) {
    epoll_event ev{.events = EPOLLIN, .data = {.fd = in_event_fd(a)}};
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, in_event_fd(a), &ev);
}

ChanStatsPair* poll_actor(Actor* actor, MsgBuffer* in, MsgBuffer* out) {

    Msg**   next  = next_msg_ref(in);
    size_t  space = remaining_space(in);
    size_t added = actor->in_->poll({next, space});
    in->head_ = (in->head_ + added) % in->cap_;

    next  = next_msg_ref(out);
    space = remaining_space(out);
    added = actor->out_->get_free_msgs({next, space});
    out->head_ = (out->head_ + added) % out->cap_;

    actor->clear_in_event();
    return actor->poll_telemetry();
}

void update_actor(Actor* actor, size_t in_processed, size_t out_pending) {
    // flush checkedout msgs from in
    actor->in_->free_msgs(in_processed);

    // pops from free and puts each msg into correct q_[msg->priority]
    actor->out_->use_free_msgs(out_pending);
    actor->write_out_event();
}



//////////// ///////// ///////// /////////////////
// FOR INTERNAL LIBRARY USE -> IN MAIN SZ_loop //
//////////// ///////// ///////// ///////////////

void update_actor_main_loop(Actor* actor, size_t* in_pending, size_t* out_processed) {
    actor->out_->free_msgs(*out_processed);
    actor->in_->use_free_msgs(*in_pending);
    *out_processed = 0;
    *in_pending = 0;
    actor->write_in_event();
}

bool poll_actor_main_loop(Actor* actor, MsgBuffer* in, MsgBuffer* out) {
    Msg**   next  = next_msg_ref(in);
    size_t  space = remaining_space(in);
    size_t  added = actor->in_->get_free_msgs({next, space});
    in->head_ = (in->head_ + added) % in->cap_;

    next  = next_msg_ref(out);
    space = remaining_space(out);
    added = actor->out_->poll({next, space});
    out->head_ = (out->head_ + added) % out->cap_;

    actor->clear_out_event();
    return added > 0;
}

