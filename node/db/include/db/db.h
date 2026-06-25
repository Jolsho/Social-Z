#pragma once
#include "api/actor.h"
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DB DB;

struct DBConfig {
    size_t          msgs_cap;
    size_t          map_size;
};

ActorThread* start_db(Actor* actor, DBConfig* conf);


#ifdef __cplusplus
}
#endif
