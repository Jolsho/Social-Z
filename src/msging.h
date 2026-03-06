#pragma once
#include <sys/epoll.h>
#include <vector>
#include <atomic>
#include <sys/eventfd.h>
#include <unistd.h>
#include "net/net.h"

namespace msging {

class SPSCQueue {
private:
    std::vector<net::Packet*> buffer_;
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

    bool push(net::Packet* item) {
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

    net::Packet* pop() {
        size_t head = head_.load(std::memory_order_relaxed);

        if (head == tail_.load(std::memory_order_acquire)) {
            return nullptr;
        }

        net::Packet* item = buffer_[head];
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

inline ChannelPair new_channel(uint64_t size) {
    return {
        msging::SPSCQueue{size}, 
        msging::SPSCQueue{size}
    };
}

inline void register_queue(
    int epoll_fd, msging::SPSCQueue& q
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
