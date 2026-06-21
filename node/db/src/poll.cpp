#include "db.h"
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

                auto stats = poll_actor(chans_, in_msgs_, free_out_msgs_);
                if (stats != NULL) logr_->log(stats);

                while (Msg* msg = consume_msg(in_msgs_)) {

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
                        Msg* rm = consume_msg(free_out_msgs_);
                        if (rm) {
                            *rm  = *msg;
                            rm->priority = PRIORITY_CONT;
                        } else {
                            free(msg->data->b);
                            delete msg->data;
                        }
                    }
                }

                update_actor(chans_, &in_msgs_->consumed_, &free_out_msgs_->consumed_);
            }
        }
    }
}

