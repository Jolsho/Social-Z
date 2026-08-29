/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_node/actor.h"
#include "sz_node/msgT.h"
#include "sz_common/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Channel {

    size_t      budgets_    [PRIORITY_COUNT];
    Msg*        msgs_;

    SPSCQueue   q_          [PRIORITY_COUNT];
    SPSCQueue   free_;
    SPSCQueue   in_use_;

    size_t      current_;
    int         counts_     [PRIORITY_COUNT];

    uint64_t    accepted_          [PRIORITY_COUNT];
    uint64_t    dropped_           [PRIORITY_COUNT];
    uint64_t    dropped_last_log_  [PRIORITY_COUNT];
    uint64_t    accepted_last_log_ [PRIORITY_COUNT];

} Channel;


// CHAN_SIZE => sizes * (sizeof(Msg) + sizeof(Msg*) * chan(q_,free_,in_use_))
// 1024 * (32 + 8*3) => 58kb

// (actors * (58k + budgets) * actor(in_, out_))
// (9 * (58k + 512) * 2) => 1mb
Channel* new_channel(
    size_t msg_cap, Actors parent, 
    const ChannelSizes* sizes,
    const ChannelSizes* budgets
);

void delete_channel(Channel* c);
bool has_space(Channel* c, Priority p);
void change_budget(Channel* c, size_t idx, size_t bud);
void poll(Channel* c, MsgBuffer* msgs);
void get_free_msgs(Channel* c, MsgBuffer* msgs);

void free_msgs(Channel* c, size_t size);
void use_free_msgs(Channel* c, size_t size);
QueueStats stats(Channel* c, Priority p);
ChanSetStats snapshot(Channel* c);

#ifdef __cplusplus
}
#endif



