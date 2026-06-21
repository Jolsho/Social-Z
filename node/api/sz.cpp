#include <array>
#include <cassert>
#include "api/sz.h"
#include "utils/shutdown.h"
#include <cstddef>
#include <cstdlib>
#include <sys/epoll.h>

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

    int fd_ = 0;
    std::array<ActorMainView, ACTOR_COUNT>    actors_ = {};
};

    
SZT* new_sz() {
    SZT* sz = new SZT();
    sz->fd_ = epoll_create(0);
    return sz;
}

void stop(SZT* sz) { should_shutdown = true; }

bool set_actor(SZT* sz, Actors idx, Actor* actor) {
    if (idx >= ACTOR_COUNT || !actor) return false;
    sz->actors_[idx] = {
        .a = actor,
        .out_event_fd = out_event_fd(actor),
        .out = new_msg_buffer(512),
        .free_in = new_msg_buffer(512),
    };
    return true;
}

int run(SZT* sz) {

    // ENSURE EVERY ACTOR IS BEING HANDLED
    // AND REGISTER THEIR OUT WITH EPOLL
    for (auto& a: sz->actors_) { 
        if (!a.a) return -1;
        register_actor_output_with_epoll(a.out_event_fd, a.a);
    }


    const size_t MAX_EVENTS = 128;
    epoll_event events[MAX_EVENTS];

    while (true) {
        if (should_shutdown) { return 0; }

        int n = epoll_wait(sz->fd_, events, MAX_EVENTS, -1);
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
