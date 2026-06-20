#pragma once
#include <cstddef>
#include <cstdint>
#include <ctime>

typedef uint16_t ConnID;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SZT SZT;
typedef struct Actor Actor;
typedef int SSID;
typedef int TrxID;

#define HASH_SIZE 32
struct HashT {
    unsigned char b[HASH_SIZE];
};



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


typedef uint8_t Priority; 
#define PRIORITY_CRIT   0
#define PRIORITY_CONT   1
#define PRIORITY_WORK   2
#define PRIORITY_TELE   3
#define PRIORITY_COUNT  4


typedef uint8_t Noti;
#define NOTI_CARD       1
#define NOTI_GIVE       2
#define NOTI_ACCEPTED   3 
#define NOTI_DECLINED   4
#define NOTI_PERM_REQ   5



struct Vec {
    unsigned char*  b;
    unsigned char*  c;
    size_t          len;
    size_t          cap;
};

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
};
MsgBuffer* new_msg_buffer(size_t cap);
Msg* next_msg(MsgBuffer* buff);
Msg** next_msg_ref(MsgBuffer* buff);
void revert_msg(MsgBuffer* buff);
size_t remaining_space(MsgBuffer* buff);
size_t element_count(MsgBuffer* buff);

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

SZT* new_sz();
int run(SZT* sz);
void stop(SZT* sz);
bool set_actor(SZT* sz, Actors idx, Actor* actor);

Actor* new_actor(ActorConfig* config);
int in_event_fd(Actor* a);
int out_event_fd(Actor* a);
int register_actor_output_with_epoll(int epoll_fd, Actor* a);
int register_actor_input_with_epoll(int epoll_fd, Actor* a);
ChanStatsPair* poll_actor(Actor* a, MsgBuffer* in, MsgBuffer* out);
void update_actor(Actor* a, size_t in_processed, size_t out_pending);

// THESE CAN JUST BE HELPER FUNCTIONS...
// IF YOU WANT TO USE THEM YOU CAN BUT YOU DONT NEED TO...
// IF YOU CARE ABOUT PERFORMANCE DONT USE THESE
// Just implement them yourselve in your native language.

// AUTH
void get_challenge(SZT* sz, char** challenge, size_t* len);
SSID login_user(SZT* sz, const char* key, const char* signature);
SSID register_user(SZT* sz, const char* key, const char* signature);
void logout_user(SZT* sz, SSID ssid);


// THESE PASS MSGS TO SZT ACTORS
// POP FROM SZT MSG STORE AND PUSH TO DESTINATION

HashT new_file(SZT* sz, SSID ssid, size_t total_len);
bool file_chunk(SZT* sz, TrxID trx_id, const unsigned char* data, size_t len);
TrxID get_file(SZT* sz, SSID ssid, HashT* name);


HashT put_card(SZT* sz, SSID ssid, const unsigned char* data, size_t len, int cardType);
TrxID remove_card(SZT* sz, SSID ssid, const char* id);


// Broadcast Voucher for Card.
// list of pubkeys and a single card hash.
// I think you just add something like a broadcast id to the connection.
// so when the message finishes sending then you can access man.broadcast
//      to send to the next user or whatever.

// Request card[] from remote user
// AKA Redeem voucher || Temp Request

typedef uint8_t DB_PATH; 
#define DB_USER_INSERT      0
#define DB_USER_DELETE      1
#define DB_CARD_RECENT      2
#define DB_PERM_INSERT      3
#define DB_PERM_DELETE      4
#define DB_VOUCHER_INSERT   5
#define DB_NEW_BLOB         6


struct Card {
    int     id;
    HashT   hash;
    int     created_at;
};

#ifdef __cplusplus
}
#endif
