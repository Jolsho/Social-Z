#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

typedef uint16_t ConnID;

#define MAX_BUFFER_SIZE (1024 * 4)

typedef struct Msg {
    bool        is_wiped;
    uint8_t     too;
    uint8_t     from;

    ConnID      id;
    int         mid;
    uint16_t    code;

    unsigned char*    data;
    size_t      data_len;
    size_t      data_cap;
} Msg;

void msg_init(Msg* m, uint8_t from, size_t cap);
void msg_wipe(Msg* m);
void msg_destroy(Msg* m);
int msg_resize(Msg* m, size_t new_size);
inline int msg_insert(Msg* m, const uint8_t* src, size_t n) {
    if (m->data_len + n > m->data_cap) {
        return -1;
    }
    memcpy(m->data + m->data_len, src, n);
    m->data_len += n;
    return 0;
}
