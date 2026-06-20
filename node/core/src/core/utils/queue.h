#pragma once
#include <cstdlib>
#include <sys/eventfd.h>
#include <vector>
#include <atomic>

template <typename T>
class SPSCQueue {
private:
    std::vector<T> buffer_;
    size_t capacity_;

    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};


public:
    explicit SPSCQueue(size_t capacity, uint8_t parent = 0)
        : capacity_(capacity + 1),
        buffer_(capacity)
    {}
    

    bool has_space() {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t next = (tail + 1) % capacity_;

        return next != head_.load(std::memory_order_acquire);
    }

    T* reserve() {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next = (head + 1) % capacity_;

        if (next == tail_.load(std::memory_order_acquire)) {
            return nullptr; // full
        }

        return &buffer_[head];
    }

    void commit() {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next = (head + 1) % capacity_;
        head_.store(next, std::memory_order_release);
    }

    T* front() {
        size_t head = head_.load(std::memory_order_relaxed);
        if (head == tail_.load(std::memory_order_acquire)) { return NULL; }
        return &buffer_[head];
    }

    void pop_front() {
        size_t head = head_.load(std::memory_order_relaxed);
        if (head == tail_.load(std::memory_order_acquire)) { return; }
        head_.store((head + 1) % capacity_, std::memory_order_release);
    }

    T* back() {
        size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) { return NULL; }
        return &buffer_[tail];
    }

    void pop_back() {
        size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) { return; }
        tail_.store((tail + 1) % capacity_, std::memory_order_release);
    }

    inline T* step(T* tail, size_t steps = 1) {
        T* head = &buffer_[head_.load(std::memory_order_relaxed)];
        for (;0 < steps; steps--) {
            if (tail == head || ++tail == head) return NULL;
        }
        return tail;
    }

    size_t take_elements(SPSCQueue<T>& obs, size_t cap) {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t tail = tail_.load(std::memory_order_relaxed);

        size_t i = 0;
        while (tail != head && i < cap) {
            T* o = obs.reserve();
            if (!o) break;

            *o = buffer_[tail+i];
            obs.commit();

            i++;
            tail = (tail + 1) % capacity_;
        }
        tail_.store(tail, std::memory_order_relaxed);
        
        return i;
    }

    inline size_t cap() { 
        return capacity_; 
    }
};
