#pragma once
#include <sys/epoll.h>
#include <atomic>
#include <vector>
#include <sys/eventfd.h>
#include <unistd.h>
#include "types.h"

enum CODE : uint16_t {
    CTRL            = 0,
    SUCCESS         = 1,

    INTERNAL        = 500,
    SHUTDOWN        = 501,
    CLOSE_CONN      = 502,
    NEW_CONN        = 503,
    WRITE           = 504,
    

    NET             = 1000,
    SYN             = 1001,
    SYNACK          = 1002,
    ACK             = 1003,


    FS              = 1500,
    NEW_BLOB        = 1501,
    GIVE            = 1502,
    SETTLE_GIVE     = 1503,
    ASK             = 1504,
    REVOKE          = 1505,
    REQUEST         = 1506,
    RESPONSE        = 1507,


    RPC             = 2000,

    BLOCK           = 2500,

    ERRORS          = 3500,
    E_OVERSIZED     = 3501,
    E_INTERNAL      = 3502,
    E_MALFORMED     = 3503,
    E_UNAUTHORIZED  = 3504,
};
Actors code_too_too(CODE c);

namespace msg {

class Msg {
    static constexpr size_t MAX_SIZE = 1024 * 2;

public:
    bool        is_wiped;
    Actors      too;
    Actors      from;

    ConnID      id;
    CODE        code;

    std::vector<std::byte> data;

    Msg(size_t cap = MAX_SIZE) {
        data.reserve(cap);
    }

    void wipe() {
        is_wiped = true;
        id = 0;
        code = CODE::CTRL;
        data.resize(0);
    }
};

class SPSCQueue {
private:
    std::vector<Msg*> buffer_;
    size_t capacity_;

    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};

    int event_fd_;

public:
    explicit SPSCQueue(size_t capacity)
        : capacity_(capacity + 1),
          buffer_(capacity + 1)
    {
        event_fd_ = eventfd(0, EFD_NONBLOCK);
    }

    ~SPSCQueue() {
        close(event_fd_);
    }

    bool push(Msg* item) {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (tail + 1) % capacity_;

        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;
        }

        bool was_empty = (tail == head_.load(std::memory_order_acquire));

        buffer_[tail] = item;
        tail_.store(next_tail, std::memory_order_release);

        if (was_empty) {
            uint64_t one = 1;
            write(event_fd_, &one, sizeof(one));
        }

        return true;
    }

    Msg* pop() {
        size_t head = head_.load(std::memory_order_relaxed);

        if (head == tail_.load(std::memory_order_acquire)) {
            return nullptr;
        }

        Msg* item = buffer_[head];
        buffer_[head] = nullptr;

        head_.store((head + 1) % capacity_, std::memory_order_release);
        return item;
    }

    int get_event_fd() const {
        return event_fd_;
    }

    void clear_event() {
        uint64_t val;
        read(event_fd_, &val, sizeof(val));
    }
};

struct ChannelPair {
    SPSCQueue from;
    SPSCQueue to;
};

inline void register_queue(
    int epoll_fd, SPSCQueue& q
) {
    epoll_event ev{
        .events = EPOLLIN, 
        .data = {
            .fd =q.get_event_fd()
        },
    };
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, q.get_event_fd(), &ev);
}
}
