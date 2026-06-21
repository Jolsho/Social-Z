#pragma once
#include "msgT.h"
#include <ctime>

#ifdef __cplusplus
extern "C" {
#endif

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
#define ACTOR_BC    5
#define ACTOR_COUNT 6
#define ACTOR_NONE  7

struct ActorConfig {
    Actors  id;

    size_t* in_q_sizes; 
    size_t  in_q_sizes_len;

    size_t* in_budgets; 
    size_t  in_budgets_len;

    size_t* out_q_sizes; 
    size_t  out_q_sizes_len;

    size_t* out_budgets; 
    size_t  out_budgets_len;
};

Actor* new_actor(ActorConfig* config);

int in_event_fd(Actor* a);
int out_event_fd(Actor* a);

int register_actor_output_with_epoll(int epoll_fd, Actor* a);
int register_actor_input_with_epoll(int epoll_fd, Actor* a);

ChanStatsPair* poll_actor(Actor* a, MsgBuffer* in, MsgBuffer* out);
void update_actor(Actor* a, size_t* in_processed, size_t* out_pending);

bool poll_actor_main_loop(Actor* actor, MsgBuffer* in, MsgBuffer* out);
void update_actor_main_loop(Actor* actor, size_t* in_pending, size_t* out_processed);

#ifdef __cplusplus
}
#endif
