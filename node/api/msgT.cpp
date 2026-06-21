#include "api/msgT.h"
#include <cstddef>
#include <cstdlib>

Msg* msg_new(uint8_t from, size_t cap, unsigned char* bytes) {
    Msg* m = new Msg{};
    m->is_wiped = false;
    m->too = 0;
    m->from = from;

    m->id = 0;
    m->code = 0;

    if (!bytes) {
        m->data->b = (uint8_t*)malloc(cap);
    } else {
        m->data->b = bytes;
    }
    m->data->len = 0;
    m->data->cap = cap;

    return m;
}

void msg_wipe(Msg* m) {
    m->is_wiped = true;
    m->too = 0;
    m->id = 0;
    m->code = -1;
    if (m->data) m->data->len = 0;
    m->priority = PRIORITY_COUNT;
}

int msg_resize(Msg* m, size_t new_cap) {
    if (!m->data) return -1;

    if (new_cap <= m->data->cap) {
        m->data->len = new_cap;
        return 0;
    }

    uint8_t* next = (uint8_t*)realloc(m->data, new_cap);

    if (!next)
        return -1;

    m->data->b = next;
    m->data->cap = new_cap;

    if (m->data->len > new_cap)
        m->data->len = new_cap;

    return 0;
}


MsgBuffer* new_msg_buffer(size_t cap) {
    MsgBuffer* buff = new MsgBuffer();
    buff->cap_ = cap;
    buff->msgs_ = (Msg**)malloc(sizeof(Msg*) * cap);
    return buff;
}

void delete_msg_buffer(MsgBuffer* buff) {
    if (buff->msgs_) free(buff->msgs_);
    delete buff;
}

Msg* consume_msg(MsgBuffer* buff) {
    if (buff->tail_ == buff->head_) return NULL;
    Msg* m = *(buff->msgs_ + buff->tail_); // From back
    buff->tail_ = (buff->tail_ + 1) % buff->cap_;
    buff->consumed_++;
    return m;
}
void unconsume_msg(MsgBuffer* buff) {
    size_t prev = (buff->tail_ - 1) % buff->cap_;
    if (buff->head_ == prev) return; 
    buff->tail_ = prev;
    buff->consumed_--;
}

Msg* reserve_msg(MsgBuffer* buff) {
    size_t next = (buff->head_ + 1) % buff->cap_;
    if (buff->tail_ == next) return NULL;
    Msg* m = *(buff->msgs_ + buff->head_); // From front
    buff->head_ = next;
    return m;
}

void release_msg(MsgBuffer* buff) {
    if (buff->tail_ == buff->head_) return;
    size_t prev = (buff->head_ - 1) % buff->cap_;
    buff->head_ = prev;
}

size_t remaining_space(MsgBuffer* buff) {
    return buff->cap_ - ((buff->head_ + buff->cap_ - buff->tail_) % buff->cap_);
}

size_t element_count(MsgBuffer* buff) {
    return (buff->head_ + buff->cap_ - buff->tail_) % buff->cap_;
}
