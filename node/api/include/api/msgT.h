#pragma once
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t Priority; 
#define PRIORITY_CRIT   0
#define PRIORITY_CONT   1
#define PRIORITY_WORK   2
#define PRIORITY_TELE   3
#define PRIORITY_COUNT  4


typedef struct Msg {
    bool        is_wiped;
    uint8_t     too;
    uint8_t     from;

    ConnID      id;
    int         code;
    size_t      priority;

    Vec*        data;

} Msg;
Msg* msg_new(uint8_t from, size_t cap, unsigned char* bytes);
void msg_wipe(Msg* m);
int msg_resize(Msg* m, size_t new_size);




struct MsgBuffer {
    Msg**   msgs_   = nullptr;
    size_t  head_   = 0;
    size_t  tail_   = 0;
    size_t  cap_    = 0;
    size_t  consumed_   = 0;
};
MsgBuffer* new_msg_buffer(size_t cap);
Msg* consume_msg(MsgBuffer* buff);
void unconsume_msg(MsgBuffer* buff);
Msg* reserve_msg(MsgBuffer* buff);
void release_msg(MsgBuffer* buff);
size_t remaining_space(MsgBuffer* buff);
size_t element_count(MsgBuffer* buff);

#ifdef __cplusplus
}
#endif
