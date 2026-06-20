#include "bindings.h"

bool poll_actor_main_loop(
    Actor* actor, 
    MsgBuffer* in,
    MsgBuffer* out
);

void update_actor_main_loop(
    Actor* actor, 
    size_t* in_pending, 
    size_t* out_processed
);


