#include "sz/api/sz.h"
#include "sz/api/msgT.h"
#include "sz/utils/shutdown.h"
#include <stdlib.h>
#include <sys/epoll.h>

typedef struct ActorMainView {
    Actor*      a;
    int         out_event_fd;

    MsgBuffer*  out;
    size_t      out_processed;

    MsgBuffer*  free_in;
    size_t      in_pending;

    bool        has_work;
} ActorMainView;

typedef struct SZT {
    int             epoll_fd_;
    ActorMainView   actors_[ACTOR_COUNT];
} SZT;

    
SZT* new_sz() {
    SZT* sz = (SZT*)malloc(sizeof(SZT));
    sz->epoll_fd_ = epoll_create(0);
    return sz;
}

void stop(SZT* sz) { sz_shutdown(); }

Actor* new_actor(SZT* sz, ActorConfig* conf) {
    Actor* actor = create_actor(conf);
    if (!actor) return NULL;

    if (conf->id >= ACTOR_COUNT) {
        delete_actor(actor);
        return NULL;
    }
    ActorMainView* av = & sz->actors_[conf->id];
    av->a = actor;
    av->out_event_fd = actor->out_event_fd_;
    av->out = new_msg_buffer(512);
    av->free_in = new_msg_buffer(512);
    return actor;
}

int run(SZT* sz) {

    // ENSURE EVERY ACTOR IS BEING HANDLED
    // AND REGISTER THEIR OUT WITH EPOLL
    for (int i = 0; i < ACTOR_COUNT; i++) { 
        ActorMainView* a = &sz->actors_[i];

        if (!a->a) return -1;

        struct epoll_event ev = {
            .events = EPOLLIN, .data = { .fd = a->out_event_fd }
        };
        int r = epoll_ctl(sz->epoll_fd_, EPOLL_CTL_ADD, a->out_event_fd, &ev);
        if (r < 0) return r;
    }


    const size_t MAX_EVENTS = 128;
    struct epoll_event events[MAX_EVENTS];

    while (true) {
        if (should_shutdown()) { return 0; }

        int n = epoll_wait(sz->epoll_fd_, events, MAX_EVENTS, 3000);
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            // POLL ACTORS WHICH HAVE NEW EVENTS
            for (int k = 0; k < ACTOR_COUNT; k++) {
                ActorMainView* from = &sz->actors_[k];
                if (fd == from->out_event_fd) {
                    from->has_work = poll_actor_main_loop(from->a, from->free_in, from->out);
                }
            }
        }


        // DISPATCH MESSAGES AMONG ACTORS
        for (int k = 0; k < ACTOR_COUNT; k++) {
            ActorMainView* from = &sz->actors_[k];

            if (!from->has_work) continue;
            from->has_work = false;

            Msg* msg = consume_msg(from->out);
            while (msg) {

                from->out_processed++;

                if (msg->too >= ACTOR_COUNT) {
                    if (msg->data) {
                        free(msg->data->b);
                        free(msg->data);
                    }
                    continue;
                }

                ActorMainView* to = &sz->actors_[msg->too];
                Msg* to_msg = consume_msg(to->free_in);
                if (to_msg) {
                    to->in_pending++;
                    *to_msg = *msg;

                } else {
                    Msg* return_msg = consume_msg(from->free_in);
                    if (return_msg) {
                    
                        // TRY TO RETURN TO SENDER
                        from->in_pending++;
                        *return_msg = *msg;
                        return_msg->priority = PRIORITY_CONT;
                        msg_wipe(return_msg);
                    } else {
                        if (msg->data) {
                            free(msg->data->b);
                            free(msg->data);
                        }
                    }
                }

                msg = consume_msg(from->out);
            }
        }

        // UPDATE ACTOR INTERNAL STATE FLUSHING MSGS
        for (int k = 0; k < ACTOR_COUNT; k++) {
            ActorMainView* from = &sz->actors_[k];
            if (from->in_pending == 0 && from->out_processed == 0) continue;
            update_actor_main_loop(from->a, &from->in_pending, &from->out_processed);
        }
    }
    return 0;
}
