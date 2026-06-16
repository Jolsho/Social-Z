#include "loop.h"
#include "utils/chans.h"
#include "utils/shutdown.h"

template<size_t S>
void main_loop(
    int main_epoll_fd, 
    std::array<ActorChannel, S>& actors,
    std::function<void (Msg*)> callback
) {
    const size_t MAX_EVENTS = 64;
    while (true) {
        if (should_shutdown) { return; }

        epoll_event events[MAX_EVENTS];
        int n = epoll_wait(main_epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            // DISPATCH MESSAGES AMONG ACTORS
            for (ActorChannel &from: actors) {
                if (fd == from.get_event_fd()) {
                    from.clear_event();
                    Msg* msg;

                    while (from.out_.front(msg)) {

                        if (callback) callback(msg);
                        Priority prio = priority(msg->priority);

                        if (msg->too < S) {
                            ActorChannel& to = actors[msg->too];

                            Msg* to_msg = to.in_.reserve(prio);
                            if (!to_msg) {

                                // TRY TO RETURN TO SENDER
                                Msg* return_msg = from.in_.reserve(Priority::Control);
                                if (!return_msg) {
                                    if (msg->data) {
                                        free(msg->data->b);
                                        delete msg->data;
                                    }
                                } else {
                                    *return_msg = *msg;
                                    from.in_.commit(Priority::Control);
                                }


                            } else {
                                *to_msg = *msg;
                                to.in_.commit(prio);
                            }

                        } else {
                            if (msg->data) {
                                free(msg->data->b);
                                delete msg->data;
                            }
                        }
                        from.out_.pop();
                    }
                }
            }
            // ANY OTHER THINGS?
        }
    }
}
