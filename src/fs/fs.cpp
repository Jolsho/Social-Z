#include "fs/fs.h"

fs::Manager::Manager(msg::ChannelPair& chan, const char* path, size_t map_size) : 
    from_main_(chan.to), 
    too_main_(chan.from), 
    db_(path, map_size)
{
    msgs_.reserve(64);
    open_files_.reserve(64);
    sessions_.reserve(64);


    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) perror("epoll_create1");
}


void fs::Manager::poll_loop() {
    const int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];
    while (true) {
        // short timeout
        int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, 0); 
        time_t now = time(nullptr);

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            if (fd == from_main_.get_event_fd()) {
                from_main_.clear_event();

                int k = 0;
                while (auto* msg = from_main_.pop()) {
                    if (!msg->is_wiped) {
                        switch (msg->code) {
                            case CODE::VOUCHER:     voucher(msg);    break;
                            case CODE::REDEEM:      redeem(msg);   break;
                            case CODE::REWARD:      reward(msg);  break;

                            case CODE::GIVE:        give(msg);      break;
                            case CODE::ACCEPT:      accept(msg);    break;
                            case CODE::ASK:         ask(msg);       break;
                            case CODE::REVOKE:      revoke(msg);    break;
                            default: break;
                        }
                    }

                    if (!msg->is_wiped) msg->wipe();

                    if (msg->from == Actors::FILESYS) {
                        if (msgs_.size() < msgs_.capacity()) {
                            msgs_.push_back(msg);
                        } else {
                            delete msg;
                        }
                    } else {
                        if (!too_main_.push(msg)) {
                            delete msg;
                        }
                    }
                    if (++k > 32) break;

                }
            } else {
                // OTHER FD
            }
        }

        // AFTER PROCESSING EVENTS

    }
}
