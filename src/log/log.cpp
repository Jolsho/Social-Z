#include "log.h"
#include <cstring>
#include <fcntl.h>

Logger::Logger(ActorChannels& chan, LogConfig& conf) : 
    from_main_(chan.to),
    to_main_(chan.from)
{
    msgs_.reserve(conf.msgs_cap);
}

int Logger::initialize() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        perror("epoll_fd");
        return -1;
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = from_main_.get_event_fd();
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


std::string Logger::derive_file_name() {
    char buf[64];

    std::time_t now = std::time(nullptr);
    std::tm tm{};
    localtime_r(&now, &tm);

    std::strftime(buf, sizeof(buf), "log_%Y%m%d_%H%M%S.txt", &tm);
    return std::string(buf);
}

bool Logger::write_log(Msg* l) {
    if (!f_ || !l) return false;
    std::byte* b = reinterpret_cast<std::byte*>(l->data);
    size_t n = write(f_, b, l->data_len);
    if (n < l->data_len) {
        size_t remaining = l->data_len - n;
        memmove(b, b + n, remaining);
        l->data_len = remaining;
        return false;
    }
    return true;
}

void Logger::poll_loop() {
    const int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];
    while (true) {
        // short timeout
        int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, 2); 
        if (n == -1) {
            perror("Epoll_wait");
            continue;
        }

        time_t now = time(nullptr);

        int counter = 0;
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            if (fd == from_main_.get_event_fd()) {
                from_main_.clear_event();

                int k = 0;
                while (auto* msg = from_main_.pop()) {

                    if (!msg->is_wiped) write_log(msg);

                    if (!msg->is_wiped) msg_wipe(msg);

                    if (msg->from == Actors::LOGGER) {
                        if (msgs_.size() < msgs_.capacity()) {
                            msgs_.push_back(msg);
                        } else {
                            delete msg;
                        }
                    } else {
                        if (!to_main_.push(msg)) {
                            delete msg;
                        }
                    }
                    counter++;
                    if (++k > 32) break;

                }
            } else {
                // OTHER FD
            }
        }

        // AFTER PROCESSING EVENTS

        if (counter % 64 == 0) {
            fsync(f_);
            counter = 0;
        }
    }
}

