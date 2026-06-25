#include "api/actor.h"
#include "utils/chans.h"
#include <cstdlib>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <thread>

void wait_on_actor_thread(ActorThread* at) {
    if (!at || at->r != 0) return;
    if (at->t != nullptr) ((std::thread*)at->t)->join();
}

class Actor {

private:
    time_t          next_snapshot = time(nullptr) + 20;
    ChanStatsPair   stats_;
public:
    Actors      id_;

    int         out_event_fd_;
    Channel*    out_;
    MsgBuffer   out_msg_view_ = {};

    int         in_event_fd_;
    Channel*    in_;
    MsgBuffer   in_msg_view_ = {};

    int epoll_fd_;
    std::vector<epoll_event> events_;


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

void delete_actor(Actor* a) {
    delete a->in_;
    delete a->out_;
    delete a;
}

Actor* create_actor(ActorConfig* config) {
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
        in_sizes[i] = *(c.in_q_sizes++);
        in_buds[i] = *(c.in_budgets++);


        out_msg_cap += *c.out_q_sizes;
        in_sizes[i] = *(c.out_q_sizes++);
        out_buds[i] = *(c.out_budgets++);
    }

    a->in_ = new Channel(
        config->event_cap, 
        a->id_, in_sizes, in_buds
    );
    a->events_.resize(config->event_cap);

    a->out_ = new Channel(
        out_msg_cap, 
        a->id_, out_sizes, out_buds
    );

    a->epoll_fd_ = epoll_create1(0);
    if (a->epoll_fd_ < 0) {
        delete_actor(a);
        return nullptr;
    }

    epoll_event ev{.events = EPOLLIN, .data = {.fd = a->in_event_fd_}};
    int r = epoll_ctl(a->epoll_fd_, EPOLL_CTL_ADD, a->in_event_fd_, &ev);
    if (r < 0) {
        delete_actor(a);
        return nullptr;
    }

    return a;
}


int out_event_fd(Actor* a) { return a->out_event_fd_; }

int ctl_epoll(Actor* a, EpollEvent* eev, int op) {
    epoll_event ev{ .events = eev->events };
    ev.data.fd = eev->data.fd;
    ev.data.ptr = eev->data.ptr;
    ev.data.u64 = eev->data.u64;
    ev.data.u32 = eev->data.u32;
    return epoll_ctl(a->epoll_fd_, op, ev.data.fd, &ev);
}

ChanStatsPair* poll_telemetry(Actor* a) { return a->poll_telemetry(); }

EventBuffer* new_event_buffer(size_t cap) {
    EventBuffer* eb = new EventBuffer{.cap = cap};
    eb->events = (EpollEvent*)malloc(sizeof(EpollEvent*) * cap);
    return eb;
}

void delete_event_buffer(EventBuffer* b) {
    free(b->events);
    delete b;
}

void poll_actor(Actor* actor, EventBuffer* evs, MsgBuffer* in, MsgBuffer* out, int timeout_ms) {

    int msg_fd = actor->in_event_fd_;
    int n = epoll_wait(
        actor->epoll_fd_, 
        actor->events_.data(), 
        std::min(actor->events_.size(), evs->cap), 
        timeout_ms
    );

    evs->size = n;

    if (n > 0) {
        bool is_polled = false;
        EpollEvent* ee = evs->events;
        for (int i = 0; i < n; i++) {
            epoll_event& e = actor->events_[i];

            if (e.data.fd == msg_fd) {
                if (is_polled) continue;
                is_polled = true;
                actor->in_->poll(in);
                actor->out_->get_free_msgs(out);
                actor->clear_in_event();

            } else {
                ee->events  =   e.events;
                ee->data.fd =   e.data.fd;
                ee->data.u32 =  e.data.u32;
                ee->data.u64 =  e.data.u64;
                ee->data.ptr =  e.data.ptr;
                ee++;
            }
        }
    }
}

void update_actor(Actor* actor, size_t* in_processed, size_t* out_pending) {
    // flush checkedout msgs from in
    actor->in_->free_msgs(*in_processed);
    *in_processed = 0;

    // pops from free and puts each msg into correct q_[msg->priority]
    actor->out_->use_free_msgs(*out_pending);
    *out_pending = 0;

    actor->write_out_event();
}



//////////// ///////// ///////// /////////////////
// FOR INTERNAL LIBRARY USE -> IN MAIN SZ_loop //
//////////// ///////// ///////// ///////////////

void update_actor_main_loop(Actor* actor, size_t* in_pending, size_t* out_processed) {
    actor->out_->free_msgs(*out_processed);
    *out_processed = 0;

    actor->in_->use_free_msgs(*in_pending);
    *in_pending = 0;

    actor->write_in_event();
}

bool poll_actor_main_loop(Actor* actor, MsgBuffer* in, MsgBuffer* out) {
    actor->in_->get_free_msgs(in);
    size_t o = out->head_;
    actor->out_->poll(out);
    actor->clear_out_event();
    return o != out->head_;
}

