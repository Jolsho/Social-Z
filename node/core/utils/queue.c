#include "sz/utils/queue.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


SPSCQueue* new_queue(size_t item_size, size_t capacity, uint8_t parent) {
    SPSCQueue* q = (SPSCQueue*)malloc(sizeof(SPSCQueue));
    q->tail_ = 0;
    q->head_ = 1;
    q->item_size_ = item_size;
    q->capacity_ = capacity * item_size;
    q->buffer_ = (uint8_t*)malloc(item_size * capacity);
    return q;
};
    

bool q_has_space(SPSCQueue* q) {
    int tail = atomic_load(&q->tail_);
    size_t next = (tail + 1) % q->capacity_;
    return next != atomic_load(&q->head_);
}

void* q_reserve(SPSCQueue* q) {
    int head = atomic_load(&q->head_);
    size_t next = (head + 1) % q->capacity_;

    if (next == atomic_load(&q->tail_)) {
        return NULL; // full
    }

    return &q->buffer_[head];
}

void q_commit(SPSCQueue* q) {
    int head = atomic_load(&q->head_);
    size_t next = (head + 1) % q->capacity_;
    atomic_store(&q->head_, next);
}

void* q_front(SPSCQueue* q) {
    int head = atomic_load(&q->head_);
    if (head == atomic_load(&q->tail_)) { 
        return NULL; 
    }
    return &q->buffer_[head];
}

void q_pop_front(SPSCQueue* q) {
    int head = atomic_load(&q->head_);
    if (head == atomic_load(&q->tail_)) { return; }
    atomic_fetch_add(&q->head_, (head + 1) % q->capacity_);
}

void* q_back(SPSCQueue* q) {
    int tail = atomic_load(&q->tail_);
    if (tail == atomic_load(&q->head_)) { 
        return NULL; 
    }
    return &q->buffer_[tail];
}

void q_pop_back(SPSCQueue* q) {
    int tail = atomic_load(&q->tail_);
    if (tail == atomic_load(&q->head_)) { return; }
    atomic_fetch_add(&q->tail_, (tail + 1) % q->capacity_);
}

void* q_step(SPSCQueue* q, void* tail, size_t steps) {
    void* head = &q->buffer_[atomic_load(&q->head_)];
    for (;0 < steps; steps--) {
        if (tail == head || ++tail == head) return NULL;
    }
    return tail;
}

size_t q_take_elements(SPSCQueue* src, SPSCQueue* dst, size_t cap) {
    int head = atomic_load(&src->head_);
    int tail = atomic_load(&src->tail_);

    size_t i = 0;
    while (tail != head && i < cap) {
        void* o = q_reserve(dst);
        if (!o) break;

        memcpy((uint8_t*)o, &src->buffer_[tail+i], src->item_size_);
        q_commit(dst);

        i++;
        tail = (tail + 1) % src->capacity_;
    }
    atomic_store(&src->tail_, tail);
    
    return i;
}
