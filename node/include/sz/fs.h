#pragma once
#include "sz/api/actor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FS FS;

typedef struct FSConfig {
    size_t      msgs_cap;
    size_t      map_size;
    size_t      concurrent_sessions;
    size_t      allotted_space;
} FSConfig;

ActorThread* start_fs(Actor* actor, FSConfig* conf);

#ifdef __cplusplus
}
#endif
