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
    size_t      concurrent_sessions;
    size_t      allotted_space;
};

ActorThread* start_fs(Actor* actor, FSConfig* conf);

#define PERM_DATA_SIZE 256
struct Perm {
    Key             giver;
    Key             recipient;
    Nonce           nonce;
    unsigned char   data[PERM_DATA_SIZE];
    Signature       signature;
};
HashT hash_perm(Perm* p);


#define VOUCH_DATA_SIZE 256
struct Voucher {
    Key         to;
    Key         from;

    HashT       file_hash;
    size_t      file_size;
    time_t      expiration;

    Signature   signature;
    unsigned char   data[VOUCH_DATA_SIZE];

};
HashT hash_voucher(Voucher* v);

// TODO --maybe move this to api?


#ifdef __cplusplus
}
#endif
