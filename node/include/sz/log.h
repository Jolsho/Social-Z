
#pragma once
#include "sz/api/actor.h"

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
