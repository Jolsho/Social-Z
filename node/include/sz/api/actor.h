#pragma once
#include "sz/api/msgT.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ActorThread {
    int r;
    void* t;
}ActorThread;
void wait_on_actor_thread(ActorThread* t);

typedef struct QueueStats {
  uint64_t accepted;
  uint64_t dropped;
  float drop_rate;

  uint64_t delta_accepted;
  uint64_t delta_dropped;
  float delta_drop_rate;
}QueueStats;

typedef struct ChanSetStats {
  QueueStats critical;
  QueueStats control;
  QueueStats work;
  QueueStats telemetry;
}ChanSetStats;

typedef struct ChanStatsPair {
  time_t timestamp;
  ChanSetStats in;
  ChanSetStats out;
}ChanStatsPair;

typedef struct ChannelSizes {
    size_t critical;
    size_t control;
    size_t work;
    size_t telemetry;
} ChannelSizes;

typedef struct Actor Actor;
typedef uint8_t Actors; 
#define ACTOR_P2P   ((Actors)0)
#define ACTOR_FS    ((Actors)1)
#define ACTOR_SZ    ((Actors)2)
#define ACTOR_DB    ((Actors)3)
#define ACTOR_LOG   ((Actors)4)
#define ACTOR_LEDG  ((Actors)5)
#define ACTOR_RPC   ((Actors)6)
#define ACTOR_COUNT ((Actors)7)
#define ACTOR_NONE  ((Actors)8)

typedef struct ActorConfig {
    Actors  id;
    size_t  event_cap;

    ChannelSizes in_q_sizes; 
    ChannelSizes in_budgets; 

    ChannelSizes out_q_sizes; 
    ChannelSizes out_budgets; 

}ActorConfig;
ActorConfig default_actor_config(Actors actor, size_t events_cap);
Actor* create_actor(ActorConfig* config);
void delete_actor(Actor* a);

typedef struct EventData {
    int         fd;
    void*       ptr;
    uint32_t    u32;
    uint64_t    u64;
}EventData;
typedef struct EpollEvent {
    uint32_t        events;
    EventData       data;
}EpollEvent;

typedef struct EventBuffer {
    EpollEvent* events;
    size_t      size;
    size_t      cap;
}EventBuffer;
EventBuffer* new_event_buffer(size_t cap);
void delete_event_buffer(EventBuffer* b);

ChanStatsPair* poll_telemetry(Actor* a);
void poll_actor(Actor* actor, EventBuffer* evs, MsgBuffer* in, MsgBuffer* out_free_msgs, int timeout_ms);
void update_actor(Actor* a, size_t* in_processed, size_t* out_pending);
int ctl_epoll(Actor* a, EpollEvent* eev, int op);

#ifdef __cplusplus
}
#endif
