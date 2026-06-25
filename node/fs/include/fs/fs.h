#pragma once
#include "api/actor.h"
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FS FS;

struct FSConfig {
    size_t      msgs_cap;
    size_t      map_size;
};

ActorThread* start_fs(Actor* actor, FSConfig* conf);


#ifdef __cplusplus
}
#endif
