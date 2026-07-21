/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz/codec.h"
#include "sz/api/vec.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t Priority; 
#define PRIORITY_CRIT   0
#define PRIORITY_CONT   1
#define PRIORITY_WORK   2
#define PRIORITY_TELE   3
#define PRIORITY_COUNT  4


typedef struct Msg {
    bool        is_wiped;
    uint8_t     too;
    uint8_t     from;

    ConnID      id;
    int         code;
    size_t      priority;

    Vec*        data;

} Msg;
Msg* msg_new(uint8_t from, size_t cap, uint8_t* bytes);
void msg_wipe(Msg* m);
int msg_resize(Msg* m, size_t new_size);



typedef struct MsgBuffer {
    Msg**   msgs_;
    size_t  cursor_;
    size_t  size_;
    size_t  cap_;
} MsgBuffer;
MsgBuffer* new_msg_buffer(size_t cap);
Msg* consume_msg(MsgBuffer* buff);
void unconsume_msg(MsgBuffer* buff);

#ifdef __cplusplus
}
#endif
