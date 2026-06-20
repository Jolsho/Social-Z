#include "log.h"
#include "code.h"
#include "utils/shutdown.h"
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <sys/epoll.h>

Logger::Logger(Actor* chan, LogConfig& conf) : 
    chans_(chan)
{
}

int Logger::initialize() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        perror("epoll_fd");
        return -1;
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = in_event_fd(chans_);
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
        perror("epoll_ctl");
        return -1;
    }

    // Open log file
    std::string fname = derive_file_name();
    f_ = open(fname.data(), O_APPEND | O_CREAT, 0600);
    if (!f_) {
        perror("fopen");
        return -1;
    }
    return 0;
}

void Logger::shutdown() {
    close(epoll_fd_);
    close(f_);
}


std::string Logger::derive_file_name() {
    char buf[64];

    std::time_t now = std::time(nullptr);
    std::tm tm{};
    localtime_r(&now, &tm);

    std::strftime(buf, sizeof(buf), "log_%Y%m%d_%H%M%S.txt", &tm);
    return std::string(buf);
}

bool Logger::parse_n_write_log(Msg* l) {
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

void Logger::poll_loop() {
    const int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];

    int chans_fd = in_event_fd(chans_);

    while (true) {
        // short timeout
        int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, 2); 
        if (n == 0 && should_shutdown) {
            shutdown();
            return;
        }

        if (n == -1) {
            perror("Epoll_wait");
            continue;
        }

        time_t now = time(nullptr);

        int counter = 0;
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            if (fd == chans_fd) {

                poll_actor(chans_, in_msgs_, out_msgs_);

                size_t processed = 0;
                while (Msg* msg = next_msg(in_msgs_)) {
                    switch (msg->code) {
                        case log_code(LogCode::Log):   {
                            parse_n_write_log(msg);     
                            break;
                        }
                        default:
                            break;
                    }

                    if (!msg->is_wiped) msg_wipe(msg);

                    Msg* to_m = next_msg(out_msgs_);
                    if (!to_m) {
                        free(msg->data->b);
                        delete msg->data;
                    } else {
                        *to_m = *msg;
                        to_m->priority = PRIORITY_CONT;
                    }
                    counter++;
                }

                // OTHER FD
            }

        }

        // AFTER PROCESSING EVENTS

        if (counter % 128 == 0) {
            fsync(f_);
            counter = 0;
        }
    }
}

