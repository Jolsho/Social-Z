#pragma once
#include "msgT.h"
#include <atomic>
#include <vector>


class SPSCQueue {
private:
    std::vector<Msg> buffer_;
    size_t capacity_;

    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};


public:
    explicit SPSCQueue(size_t capacity, uint8_t priority, uint8_t parent = 0)
        : capacity_(capacity + 1),
        buffer_(capacity)
    {
        for (int i = 0; i < buffer_.size(); i++) {
            buffer_[i].priority = priority;
            buffer_[i].from = parent;
            buffer_[i].too = parent;
        }
    }

    bool has_space() {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t next = (tail + 1) % capacity_;

        return next != head_.load(std::memory_order_acquire);
    }

    Msg* reserve() {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t next = (tail + 1) % capacity_;

        if (next == head_.load(std::memory_order_acquire)) {
            return nullptr; // full
        }

        return &buffer_[tail];
    }

    void commit() {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t next = (tail + 1) % capacity_;
        tail_.store(next, std::memory_order_release);
    }

    void pop() {
        size_t head = head_.load(std::memory_order_relaxed);
        if (head == tail_.load(std::memory_order_acquire)) { return; }
        head_.store((head + 1) % capacity_, std::memory_order_release);
    }

    Msg* front() {
        size_t head = head_.load(std::memory_order_relaxed);
        // queue empty
        if (head == tail_.load(std::memory_order_acquire)) {
            return nullptr;
        }
        return &buffer_[head];
    }

    inline size_t cap() { return capacity_; }

};

