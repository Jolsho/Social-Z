#include "msg.h"
#include "codes.h"
#include <cstdlib>

void msg_init(Msg* m, uint8_t from = 0, size_t cap = MAX_BUFFER_SIZE) {
    m->is_wiped = false;
    m->too = 0;
    m->from = from;

    m->id = 0;
    m->mid = 0;
    m->code = Code::CTRL;

    m->data = (uint8_t*)malloc(cap);
    m->data_len = 0;
    m->data_cap = cap;
}

void msg_wipe(Msg* m) {
    m->is_wiped = true;
    m->too = 0;
    m->id = 0;
    m->mid = 0;
    m->code = Code::CTRL;
    m->data_len = 0;
}

void msg_destroy(Msg* m) {
    free(m->data);
}

int msg_resize(Msg* m, size_t new_cap) {
    if (new_cap <= m->data_cap) {
        m->data_len = new_cap;
        return 0;
    }

    uint8_t* next = (uint8_t*)realloc(m->data, new_cap);

    if (!next)
        return -1;

    m->data = next;
    m->data_cap = new_cap;

    if (m->data_len > new_cap)
        m->data_len = new_cap;

    return 0;
}
