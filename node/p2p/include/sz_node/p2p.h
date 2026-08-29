
/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_node/actor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct P2P P2P;

typedef struct P2PConfig {
    size_t      msgs_cap;
    size_t      pkts_cap;
    uint16_t    port;
    const char* ip;

    size_t      wave;
    size_t      broad_msgs; 
} P2PConfig;

ActorThread* start_p2p(Actor* actor, P2PConfig* conf);
bool broadcast_msg(Key* recipients, size_t recip_len);

#ifdef __cplusplus
}
#endif
