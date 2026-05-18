#pragma once
#include "msg.h"
#include <sys/epoll.h>
#include <atomic>
#include <sys/eventfd.h>
#include <unistd.h>
#include <vector>

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

    size_t cap() { return capacity_; }
};


using MsgChan = SPSCQueue;

struct ActorChannels {
    MsgChan from    { 256 };
    MsgChan to      { 256 };
    MsgChan logs    { 256 };
};

inline void register_queue(
    int epoll_fd, MsgChan& q
) {
    epoll_event ev{
        .events = EPOLLIN, 
        .data = {
            .fd =q.get_event_fd()
        },
    };
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, q.get_event_fd(), &ev);
}
