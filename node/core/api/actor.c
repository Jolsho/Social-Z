/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz/api/actor.h"
#include "sz/utils/chans.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

ActorConfig default_actor_config(Actors actor, size_t events_cap) {
    ActorConfig a = {
        .id = actor,
        .event_cap = events_cap,
        .in_q_sizes = {
            .critical   = 32,
            .control    = 128,
            .work       = 256,
            .telemetry  = 64
        },
        .in_budgets = {
            .critical   = 16,
            .control    = 64,
            .work       = 128,
            .telemetry  = 32
        },

        .out_q_sizes = {
            .critical   = 32,
            .control    = 128,
            .work       = 256,
            .telemetry  = 64
        },
        .out_budgets = {
            .critical   = 16,
            .control    = 64,
            .work       = 128,
            .telemetry  = 32
        },
    };
    return a;
}

void wait_on_actor_thread(ActorThread* at) {
    if (!at || at->r != 0) return;
    pthread_join(at->t, NULL);
}



ChanStatsPair* poll_telemetry(Actor* a) {
    time_t now = time(NULL);
    if (now < a->next_snapshot) {
        a->next_snapshot = now + 10;

        a->stats_.timestamp = now;
        a->stats_.in = snapshot(a->in_);
        a->stats_.out = snapshot(a->out_);
        return &a->stats_;
    }
    return NULL;
}

bool write_out_event(Actor* a) {
    const uint64_t value = 1;
    return write(a->out_event_fd_, &value, sizeof(uint64_t)) == sizeof(uint64_t);
}

bool write_in_event(Actor* a) {
    const uint64_t value = 1;
    return write(a->in_event_fd_, &value, sizeof(uint64_t)) == sizeof(uint64_t);
}

bool clear_in_event(Actor* a) {
    uint64_t val;
    return read(a->in_event_fd_, &val, sizeof(uint64_t)) == sizeof(uint64_t);
}

bool clear_out_event(Actor* a) {
    uint64_t val;
    return read(a->out_event_fd_, &val, sizeof(uint64_t)) == sizeof(uint64_t);
}

void delete_actor(Actor* a) {
    delete_channel(a->in_);
    delete_channel(a->out_);
    free(a);
}

Actor* create_actor(ActorConfig* config) {
    Actor* a = (Actor*)malloc(sizeof(Actor));
    a->id_ = config->id;
    a->out_event_fd_ = eventfd(0, EFD_NONBLOCK);
    a->in_event_fd_ = eventfd(0, EFD_NONBLOCK);
    a->epoll_fd_ = epoll_create1(0);

    struct epoll_event ev = {.events = EPOLLIN, .data = {.fd = a->in_event_fd_}};
    int r = epoll_ctl(a->epoll_fd_, EPOLL_CTL_ADD, a->in_event_fd_, &ev);

    if (
        a->out_event_fd_ < 0    || 
        a->in_event_fd_ < 0     ||
        a->epoll_fd_ < 0        || 
        r < 0
    ) {
        delete_actor(a);
        return NULL;
    }

    a->in_ = new_channel(config->event_cap, a->id_, &config->in_q_sizes, &config->in_budgets);
    a->events_ = (struct epoll_event*)malloc(sizeof(struct epoll_event) * config->event_cap);

    a->out_ = new_channel(config->event_cap, a->id_, &config->out_q_sizes, &config->out_budgets);
    return a;
}



int ctl_epoll(Actor* a, EpollEvent* eev, int op) {
    struct epoll_event ev = { .events = eev->events };
    ev.data.fd = eev->data.fd;
    ev.data.ptr = eev->data.ptr;
    ev.data.u64 = eev->data.u64;
    ev.data.u32 = eev->data.u32;
    return epoll_ctl(a->epoll_fd_, op, ev.data.fd, &ev);
}

EventBuffer* new_event_buffer(size_t cap) {
    EventBuffer* b = (EventBuffer*)malloc(sizeof(EventBuffer));
    b->cap = cap;
    b->events = (EpollEvent*)malloc(sizeof(EpollEvent*) * cap);
    return b;
}

void delete_event_buffer(EventBuffer* b) {
    free(b->events);
    free(b);
}

void poll_actor(Actor* actor, EventBuffer* evs, MsgBuffer* in, MsgBuffer* out, int timeout_ms) {

    int msg_fd = actor->in_event_fd_;
    int max = actor->events_size;
    if (max < evs->cap) max = evs->cap;

    int n = epoll_wait(actor->epoll_fd_, actor->events_, max, timeout_ms);

    evs->size = 0;

    if (n > 0) {
        bool is_polled = false;
        for (int i = 0; i < n; i++) {
            struct epoll_event* e = &actor->events_[i];

            if (e->data.fd == msg_fd) {
                if (is_polled) continue;
                is_polled = true;
                poll(actor->in_, in);
                get_free_msgs(actor->out_, out);
                clear_in_event(actor);

            } else {
                EpollEvent* ee = evs->events + i;
                ee->events  =   e->events;
                ee->data.fd =   e->data.fd;
                ee->data.u32 =  e->data.u32;
                ee->data.u64 =  e->data.u64;
                ee->data.ptr =  e->data.ptr;
                evs->size++;
            }
        }
    }
}

void update_actor(Actor* actor, size_t* in_processed, size_t* out_pending) {
    // flush checkedout msgs from in
    free_msgs(actor->in_, *in_processed);
    *in_processed = 0;

    // pops from free and puts each msg into correct q_[msg->priority]
    use_free_msgs(actor->out_, *out_pending);
    *out_pending = 0;

    write_out_event(actor);
}



//////////// ///////// ///////// /////////////////
// FOR INTERNAL LIBRARY USE -> IN MAIN SZ_loop //
//////////// ///////// ///////// ///////////////

void update_actor_main_loop(Actor* actor, size_t* in_pending, size_t* out_processed) {
    free_msgs(actor->out_, *out_processed);
    *out_processed = 0;

    use_free_msgs(actor->in_, *in_pending);
    *in_pending = 0;

    write_in_event(actor);
}

bool poll_actor_main_loop(Actor* actor, MsgBuffer* in, MsgBuffer* out) {
    get_free_msgs(actor->in_, in);
    size_t o = out->size_ - out->cursor_;
    poll(actor->out_, out);
    clear_out_event(actor);
    return o < (out->size_ - out->cursor_);
}

