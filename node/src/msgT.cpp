#include "msgT.h"
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
