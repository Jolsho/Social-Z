#include "fs/fs.h"
#include <format>

fs::Manager::Manager(ActorChannels& chan, FSConfig& conf) : 
    from_main_(chan.to), 
    too_main_(chan.from), 
    db_(conf.db_path, conf.map_size),
    msgs_(conf.msgs_cap, new Msg{ Actors::FILESYS })
{
    open_files_.reserve(64);
    sessions_.reserve(64);


    static constexpr time_t LOG_FLUSH_INTERVAL = 500; // ms
    logr_ = new LogAccumulator{"FS", LOG_FLUSH_INTERVAL, [&](){ return get_msg(); }};

}

int fs::Manager::initialize() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) {
        logr_->log(std::format("EPOLL_CREATE1 FAILED: %d", epoll_fd_));
        return epoll_fd_;
    };
    return 0;
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
                            case Code::VOUCHER:     voucher(msg);    break;
                            case Code::REDEEM:      redeem(msg);   break;
                            case Code::REWARD:      reward(msg);  break;

                            case Code::GIVE:        give(msg);      break;
                            case Code::ACCEPT:      accept(msg);    break;
                            case Code::ASK:         ask(msg);       break;
                            case Code::REVOKE:      revoke(msg);    break;
                            default: break;
                        }
                    }

                    if (!msg->is_wiped) msg_wipe(msg);

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
        // TODO -- what about outbound_??

    }
}
