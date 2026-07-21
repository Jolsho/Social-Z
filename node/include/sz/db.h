/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz/api/actor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DB DB;

typedef struct DBConfig {
    size_t          msgs_cap;
    size_t          map_size;
} DBConfig;

ActorThread* start_db(Actor* actor, DBConfig* conf);


#ifdef __cplusplus
}
#endif
