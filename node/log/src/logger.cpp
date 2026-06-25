#include "api/paths.h"
#include "logger.h"
#include "utils/shutdown.h"
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <sys/epoll.h>
#include <thread>

ActorThread* start_log(Actor* actor, LogConfig* conf) {
    ActorThread* at = new ActorThread{.r = 0};
    LOG* log = new LOG(actor, conf);

    at->r = log->initialize();
    if (at->r < 0) {
        delete log;
        return at;
    }
    at->t = (void*)new std::thread([&] {
        log->poll_loop();
        delete log;
    });
    return at;
}

LOG::LOG(Actor* chan, LogConfig* conf) : 
    chans_(chan)
{
}

int LOG::initialize() {

    // Open log file
    std::string fname = derive_file_name();
    f_ = open(fname.data(), O_APPEND | O_CREAT, 0600);
    if (!f_) {
        perror("fopen");
        return -1;
    }
    return 0;
}

void LOG::shutdown() {
    close(f_);
}


std::string LOG::derive_file_name() {
    char buf[64];

    std::time_t now = std::time(nullptr);
    std::tm tm{};
    localtime_r(&now, &tm);

    std::strftime(buf, sizeof(buf), "log_%Y%m%d_%H%M%S.txt", &tm);
    return std::string(buf);
}

bool LOG::parse_n_write_log(Msg* l) {
    if (!f_ || !l) return false;
    std::byte* b = reinterpret_cast<std::byte*>(l->data);
    size_t n = write(f_, b, l->data->len);
    if (n < l->data->len) {
        size_t remaining = l->data->len - n;
        memmove(b, b + n, remaining);
        l->data->len = remaining;
        return false;
    }
    return true;
}

void LOG::poll_loop() {
    const int MAX_EVENTS = 64;
    EventBuffer* events = new_event_buffer(MAX_EVENTS);

    while (true) {
        poll_actor(chans_, events, in_msgs_, free_out_msgs_, 3000);

        if (events->size < 0) {
            should_shutdown = true;
        }

        // short timeout
        if (events->size == 0 && should_shutdown) {
            shutdown();
            return;
        }
        time_t now = time(nullptr);

        int counter = 0;
        while (Msg* msg = consume_msg(in_msgs_)) {
            switch (msg->code) {
                case LOG_LOG:   {
                    parse_n_write_log(msg);     
                    break;
                }
                default:
                    break;
            }

            if (!msg->is_wiped) msg_wipe(msg);

            Msg* to_m = consume_msg(free_out_msgs_);
            if (!to_m) {
                free(msg->data->b);
                delete msg->data;
            } else {
                *to_m = *msg;
                to_m->priority = PRIORITY_CONT;
            }
            counter++;
        }
        update_actor(chans_, &in_msgs_->consumed_, &free_out_msgs_->consumed_);

        for (int i = 0; i < events->size; ++i) {
            // OTHER FDS
        }

        // AFTER PROCESSING EVENTS
        if (counter % 128 == 0) {
            fsync(f_);
            counter = 0;
        }
    }
}

