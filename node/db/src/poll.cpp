#include "db.h"
#include "bindings.h"
#include "utils/lru.h"
#include "utils/shutdown.h"
#include <sys/epoll.h>

static constexpr size_t MAX_CONNECTIONS     = 32;
static constexpr size_t CONNECTION_TIMEOUT  = 20;

void db::Server::poll_loop() {
    const int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];
    
    ConnLRU<MAX_CONNECTIONS, CONNECTION_TIMEOUT>   lru_;

    int chans_fd = in_event_fd(chans_);

    while (true) {
        int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, 0); // short timeout
        time_t now = time(nullptr);

        if (should_shutdown) {
            shutdown();
            return;
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            if (fd == chans_fd) {

                // INTERNAL MSGS

                Msg* in = nullptr;
                Msg* out = nullptr;
                size_t in_n = 0;
                size_t out_n = 0;
                std::vector<Priority> out_priorities;

                auto stats = poll_actor(chans_, in_msgs_, out_msgs_);
                if (stats != NULL) {
                    logr_->log(stats);
                }
                if (in_n > 0) {
                    out_priorities.reserve(out_n);
                }

                int i = 0;
                for (; i < in_n; i++) {
                    Msg* msg = in + i;

                    if (!msg->is_wiped && msg->code < E_SUCCESS) {

                        // HANDLE ERROR MSG
                        Error e {
                            .id     = msg->id,
                            .code   = msg->code,
                        };
                        size_t size_r = sizeof(e.r);
                        if (msg->data->len > size_r) {
                            memcpy(msg->data, &e.r, size_r);
                            e.msg.resize(msg->data->len - size_r);
                            if (e.msg.size() > 0) {
                                e.msg.copy(
                                    reinterpret_cast<char*>(msg->data->b) + size_r, 
                                    msg->data->len - size_r
                                );
                            }
                        }
                        handle_error(e);
                    }

                    if (!msg->is_wiped) msg_wipe(msg);

                    if (msg->from == ACTOR_DB) {
                        buffers_.put(msg->data);

                    } else {
                        // Return message
                        if (out_priorities.size() < n) {
                            out[out_priorities.size()]  = *msg;
                            out_priorities.push_back(PRIORITY_CONT);
                        } else {
                            free(msg->data->b);
                            delete msg->data;
                        }
                    }
                }
                // TODO
                update_actor(chans_, i, 0);
            }
        }
    }
}

