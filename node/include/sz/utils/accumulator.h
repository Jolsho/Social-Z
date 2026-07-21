/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz/utils/buffers.h"
#include "sz/api/msgT.h"
#include "sz/api/actor.h"
#include "sz/utils/buffers.h"

#ifdef __cplusplus
extern "C" {
#endif

const size_t FIXED_LOG_PART  = 30 + sizeof(uint16_t);
const size_t PARENT_SIZE     = 6;
typedef struct LogAccumulator {
    const char*     parent_str_;
    time_t          flush_time_;
    time_t          flush_interval_;
    Msg*            full_;
    size_t          full_cap_;
    size_t          full_size_;
    Msg             l_;
    BufferStore*    buffers_;
} LogAccumulator;


LogAccumulator* new_accumulator(
    const char* parent_str, 
    time_t flush_interval, 
    BufferStore* buffers,
    Actors from
);

size_t flush(LogAccumulator* l, MsgBuffer* m);
void log_msg(LogAccumulator* l, const char* msg, int r, int code);
void log_stats(LogAccumulator* l, ChanStatsPair* stats);

#ifdef __cplusplus
}
#endif



