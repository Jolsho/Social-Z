#pragma once
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SPSCQueue {
    uint8_t*    buffer_;
    size_t      capacity_;
    atomic_int  head_;
    atomic_int  tail_;
    size_t      item_size_;
} SPSCQueue;

SPSCQueue* new_queue(size_t item_size, size_t capacity, uint8_t parent);
bool q_has_space(SPSCQueue* q);
void* q_reserve(SPSCQueue* q);
void q_commit(SPSCQueue* q);
void* q_front(SPSCQueue* q);
void q_pop_front(SPSCQueue* q);
void* q_back(SPSCQueue* q);
void q_pop_back(SPSCQueue* q);
void* q_step(SPSCQueue* q, void* tail, size_t steps);
size_t q_take_elements(SPSCQueue* src, SPSCQueue* dst, size_t cap);

#ifdef __cplusplus
}
#endif



