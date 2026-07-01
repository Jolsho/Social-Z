#include <array>
#include <cassert>
#include "sz/api/sz.h"
#include "sz/utils/shutdown.h"
#include <cstddef>
#include <cstdlib>
#include <sys/epoll.h>
#include "api/internal.h"

struct ActorMainView {
    Actor*      a = nullptr;
    int         out_event_fd = -1;

    MsgBuffer*  out = nullptr;
    size_t      out_processed = 0;

    MsgBuffer*  free_in = nullptr;
    size_t      in_pending = 0;

    bool        has_work = false;
};

class SZT {
public:

    int epoll_fd_ = 0;
    std::array<ActorMainView, ACTOR_COUNT>    actors_ = {};
};

    
SZT* new_sz() {
    SZT* sz = new SZT();
    sz->epoll_fd_ = epoll_create(0);
    return sz;
}

void stop(SZT* sz) { should_shutdown = true; }

Actor* new_actor(SZT* sz, ActorConfig* conf) {
    Actor* actor = create_actor(conf);
    if (!actor) return nullptr;

    if (conf->id >= ACTOR_COUNT) {
        delete_actor(actor);
        return nullptr;
    }
    sz->actors_[conf->id] = {
        .a = actor,
        .out_event_fd = out_event_fd(actor),
        .out = new_msg_buffer(512),
        .free_in = new_msg_buffer(512),
    };
    return actor;
}

int run(SZT* sz) {

    // ENSURE EVERY ACTOR IS BEING HANDLED
    // AND REGISTER THEIR OUT WITH EPOLL
    for (auto& a: sz->actors_) { 
        if (!a.a) return -1;

        epoll_event ev{.events = EPOLLIN, .data = { .fd = a.out_event_fd }};
        int r = epoll_ctl(sz->epoll_fd_, EPOLL_CTL_ADD, a.out_event_fd, &ev);
        if (r < 0) return r;
    }


    const size_t MAX_EVENTS = 128;
    epoll_event events[MAX_EVENTS];

    while (true) {
        if (should_shutdown) { return 0; }

        int n = epoll_wait(sz->epoll_fd_, events, MAX_EVENTS, 3000);
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            // POLL ACTORS WHICH HAVE NEW EVENTS
            for (auto& from: sz->actors_) {
                if (fd == from.out_event_fd) {
                    from.has_work = poll_actor_main_loop(from.a, from.free_in, from.out);
                }
            }
        }


        // DISPATCH MESSAGES AMONG ACTORS
        for (auto& from: sz->actors_) {
            if (!from.has_work) continue;
            from.has_work = false;

            while (Msg* msg = consume_msg(from.out)) {

                from.out_processed++;

                if (msg->too >= ACTOR_COUNT) {
                    if (msg->data) {
                        free(msg->data->b);
                        delete msg->data;
                    }
                    continue;
                }

                auto& to = sz->actors_[msg->too];
                if (Msg* to_msg = consume_msg(to.free_in)) {
                    to.in_pending++;
                    *to_msg = *msg;

                } else if (Msg* return_msg = consume_msg(from.free_in)) {
                    // TRY TO RETURN TO SENDER
                    from.in_pending++;
                    *return_msg = *msg;
                    return_msg->priority = PRIORITY_CONT;
                    msg_wipe(return_msg);
                } else {
                    if (msg->data) {
                        free(msg->data->b);
                        delete msg->data;
                    }
                }
            }
        }

        // UPDATE ACTOR INTERNAL STATE FLUSHING MSGS
        for (auto& from: sz->actors_) {
            if (from.in_pending == 0 && from.out_processed == 0) continue;
            update_actor_main_loop(from.a, &from.in_pending, &from.out_processed);
        }
    }
    return 0;
}
