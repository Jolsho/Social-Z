#pragma once
#include "sz/codec.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef NATIVE

//void some_networking_func();

#endif


typedef struct {
    Key     local;
} UserCtx;

typedef struct {
    Key     remote;
    Key     sym;
    Nonce   nonce;
}CryptCtx;


CryptCtx* new_ctx(UserCtx* user, const uint8_t* head, size_t head_len);
bool decrypt(CryptCtx* ctx, const uint8_t* buf, size_t len);
bool encrypt(CryptCtx* ctx, const uint8_t* buf, size_t len);


#define OBJ_CARD 1
inline int obj_card() { return OBJ_CARD; }

#define OBJ_USER 2
inline int obj_user() { return OBJ_USER; }


typedef struct {
    uint8_t*    buf;
    size_t      cursor;
    size_t      size;
    int         obj_t;
}Iterator;

// Iterator* iterator(const uint8_t* buf, size_t len, int obj_enum);
// size_t size(Iterator* it);
// size_t remaining(Iterator* it);
//
// Card* seek_card(Iterator* it, size_t idx);
// Card* next_card(Iterator* it);
// Card* prev_card(Iterator* it);

static inline int get_id(const Card* c) { return c->id; }
static inline void set_id(Card* c, int id) { c->id = id; }
static inline int get_created_at(const Card* c) { return c->created_at; }
static inline void new_created_at(Card* c) { c->created_at = time(NULL); }
static inline HashT* get_hash(Card* c) { return &c->hash; }

#ifdef __cplusplus
}
#endif
