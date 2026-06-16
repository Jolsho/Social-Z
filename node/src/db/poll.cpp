#include "db/db.h"
#include "utils/chans.h"
#include "utils/lru.h"
#include "utils/shutdown.h"

static constexpr size_t MAX_CONNECTIONS     = 32;
static constexpr size_t CONNECTION_TIMEOUT  = 20;

void db::Server::poll_loop() {
    const int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];
    
    ConnLRU<MAX_CONNECTIONS, CONNECTION_TIMEOUT>   lru_;

    while (true) {
        int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, 0); // short timeout
        time_t now = time(nullptr);

        if (should_shutdown) {
            shutdown();
            return;
        }
        auto stats = chans_.poll_telemetry();
        if (stats) {
            logr_->log(stats);
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            if (fd == chans_.get_event_fd()) {

                // INTERNAL MSGS
                Msg* msg;
                while (chans_.in_.front(msg)) {
                    if (!msg->is_wiped && msg->code >= E_SUCCESS) {
                        if (chans_.out_.has_space(Priority::Work)) {
                            Error e{};
                            handle_msg(e, msg);
                            if (e.is_err()) handle_error(e);
                        } else {
                            free(msg->data->b);
                            delete msg->data;
                        }


                    } else if (!msg->is_wiped) {

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

                    if (msg->from == act_code(Actors::DB)) {
                        buffers_.put(msg->data);

                    } else {
                        // Return message
                        Msg* r_m = chans_.out_.reserve(Priority::Control);
                        if (r_m) {
                            *r_m  = *msg;
                            chans_.out_.commit(Priority::Control);
                        } else {
                            free(msg->data->b);
                            delete msg->data;
                        }
                    }
                    chans_.in_.pop();
                }
            }
        }
    }
}

