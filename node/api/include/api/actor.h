#pragma once
#include "msgT.h"
#include <ctime>

#ifdef __cplusplus
extern "C" {
#endif

struct ActorThread {
    int r;
    void* t;
};
void wait_on_actor_thread(ActorThread* t);

struct QueueStats {
  uint64_t accepted;
  uint64_t dropped;
  float drop_rate;

  uint64_t delta_accepted;
  uint64_t delta_dropped;
  float delta_drop_rate;
};

struct ChanSetStats {
  QueueStats critical;
  QueueStats control;
  QueueStats work;
  QueueStats telemetry;
};

struct ChanStatsPair {
  time_t timestamp;
  ChanSetStats in;
  ChanSetStats out;
};

typedef struct Actor Actor;
typedef uint8_t Actors; 
#define ACTOR_P2P   0
#define ACTOR_FS    1
#define ACTOR_SZ    2
#define ACTOR_DB    3
#define ACTOR_LOG   4
#define ACTOR_LEDG  5
#define ACTOR_COUNT 6
#define ACTOR_NONE  7

struct ActorConfig {
    Actors  id;
    size_t  event_cap;

    size_t* in_q_sizes; 
    size_t  in_q_sizes_len;

    size_t* in_budgets; 
    size_t  in_budgets_len;

    size_t* out_q_sizes; 
    size_t  out_q_sizes_len;

    size_t* out_budgets; 
    size_t  out_budgets_len;
};

Actor* create_actor(ActorConfig* config);
void delete_actor(Actor* a);

struct EpollEvent {
    uint32_t        events;
    struct {
        int         fd;
        void*       ptr;
        uint32_t    u32;
        uint64_t    u64;
    }data;
};

struct EventBuffer {
    EpollEvent* events;
    size_t      size;
    size_t      cap;
};
EventBuffer* new_event_buffer(size_t cap);
void delete_event_buffer(EventBuffer* b);

ChanStatsPair* poll_telemetry(Actor* a);
void poll_actor(Actor* actor, EventBuffer* evs, MsgBuffer* in, MsgBuffer* out_free_msgs, int timeout_ms = 0);
void update_actor(Actor* a, size_t* in_processed, size_t* out_pending);
int ctl_epoll(Actor* a, EpollEvent* eev, int op);

#ifdef __cplusplus
}
#endif
