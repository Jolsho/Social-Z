#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

typedef uint16_t ConnID;

struct Vec {
    unsigned char*  b;
    unsigned char*  c;
    size_t          len;
    size_t          cap;
};


typedef struct Msg {
    bool        is_wiped;
    uint8_t     too;
    uint8_t     from;

    ConnID      id;
    int         code;
    size_t      priority;

    Vec*        data;

} Msg;

Msg* msg_new(uint8_t from, size_t cap, unsigned char* bytes);
void msg_wipe(Msg* m);
unsigned char* msg_destroy(Msg* m);
int msg_resize(Msg* m, size_t new_size);
