#pragma once
#include "sz/codec.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef __EMSCRIPTEN__

void some_networking_func();

#endif // !__EMSCRIPTEN__



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

Iterator* iterator(const uint8_t* buf, size_t len, int obj_enum);
size_t size(Iterator* it);
size_t remaining(Iterator* it);

Card* seek(Iterator* it, size_t idx);
Card* next(Iterator* it);
Card* prev(Iterator* it);


uint32_t get_id(Card* c);
float get_x(Card* c);
float get_y(Card* c);


// NEED AN ENTIRE CLIENT STATE
// So like login returns a buffer.
// Then we need to parse that data
// to load in keys and what not

// ARRAY OF CARDS
//      Next()
//      get_field()

int defined_func();

#ifdef __cplusplus
}
#endif
