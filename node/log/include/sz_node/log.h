
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

typedef struct LOG LOG;

typedef struct LogConfig {
    size_t      msgs_cap;
} LogConfig;

ActorThread* start_log(Actor* actor, LogConfig* conf);

#ifdef __cplusplus
}
#endif
