#pragma once
#include "api/actor.h"
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct P2P P2P;

struct P2PConfig {
    size_t      msgs_cap    { 256 };
    size_t      pkts_cap    { 256 };
    uint16_t    port        { 3213 };
    const char* ip          { "0.0.0.0" };

    size_t      wave        { 25 };
    size_t      broad_msgs  { 128 }; 
};

ActorThread* start_p2p(Actor* actor, P2PConfig* conf);
bool broadcast_msg(Key* recipients, size_t recip_len);

#ifdef __cplusplus
}
#endif
