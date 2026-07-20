#include "server.hpp"
#include "sz/utils/lru.hpp"
#include "sz/utils/shutdown.h"
#include <cstring>
#include <stdlib.h>
#include <sys/epoll.h>

static constexpr size_t MAX_CONNECTIONS     = 32;
static constexpr size_t CONNECTION_TIMEOUT  = 20;

void DB::poll_loop() {
    ConnLRU<MAX_CONNECTIONS, CONNECTION_TIMEOUT>   lru_ {};

    const int MAX_EVENTS = 64;
    EventBuffer* events = new_event_buffer(MAX_EVENTS);

    while (true) {
        // short timeout
        poll_actor(chans_, events, in_msgs_, free_out_msgs_, 200);

        if (events->size < 0) {
            sz_shutdown();
        }

        if (should_shutdown()) {
            sz_shutdown(); 
            return;
        }

        time_t now = time(nullptr);

        while (Msg* msg = consume_msg(in_msgs_)) {

            if (!msg->is_wiped && msg->code < E_SUCCESS) {

                // HANDLE ERROR MSG
                Error e {
                    .id     = msg->id,
                    .code   = msg->code,
                };

                size_t size_r = sizeof(e.r);
                if (msg->data->len > size_r) {
                    memcpy(&e.r, msg->data, size_r);
                    size_t s = msg->data->len - size_r + 1;
                    if (e.msg && strlen(e.msg) > s) {
                        free((char*)e.msg);
                        char* m = (char*)malloc(s);
                        *(m + s) = '\0';
                        e.msg = m;
                    } else if (!e.msg) {
                        char* m = (char*)malloc(s);
                        *(m + s) = '\0';
                        e.msg = m;
                    }
                    memcpy(&e.msg, msg->data, msg->data->len);
                }
                handle_error(e);
            }

            if (!msg->is_wiped) msg_wipe(msg);

            if (msg->from == ACTOR_DB) {
                put_buff(buffers_, msg->data);

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

        auto stats = poll_telemetry(chans_);
        if (stats != NULL) log_stats(logr_, stats);

        update_actor(chans_, &in_msgs_->cursor_, &free_out_msgs_->cursor_);

    }
}

