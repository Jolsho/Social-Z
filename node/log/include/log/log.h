#pragma once
#include "api/actor.h"
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LOG LOG;

struct LogConfig {
    size_t      msgs_cap;
};

ActorThread* start_log(Actor* actor, LogConfig* conf);

#ifdef __cplusplus
}
#endif
