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


/*
 *  Create a user context when logging in.
 *  So this is either take in keys or name and password
 *      Maybe look for stored stuff....
 *      This would be on native.
 *      On web it would be different.
 *      Ill have to write that code.
 *
 *  I think most of this stuff should just be marshal and unmarshal.
 *  And just large buffer things.
 *  Which I do know to be true anyway.
 *  I think to be honest.
 *  We do a login, with username and password or just a private key.
 *      that creates initial part of a user context.
 *      then we request data from server.
 *      when that comes back we can parse it.
 *
 *  Encrypt Card, keep track of key.
 *  Encrypt key using shared key(s) with local_auth and remote_auth
 *      given buffer of recipients and enc context
 *
 *  I like the idea of just sending packets straight to remote nodes through proxies.
 *  Like what if I can like encrypt a packet then send the packet through a proxy.
 *  Such that I can communicate through a proxy to a remote node.
 *  That is kinda what going on.
 *  I mean you dont do it with meta data like vouchers and shit.
 *  but you could do that.
 *  This way proxies cant really tell what you are doing exactly.
 *      Hypothetically you could do onion routing as well.
*/

typedef struct {
    KeyPair     keys;
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

Card* seek_card(Iterator* it, size_t idx);
Card* next_card(Iterator* it);
Card* prev_card(Iterator* it);

static inline int get_id(const Card* c) { return c->id; }
static inline void set_id(Card* c, int id) { c->id = id; }
static inline int get_created_at(const Card* c) { return c->created_at; }
static inline void new_created_at(Card* c) { c->created_at = time(NULL); }
static inline HashT* get_hash(Card* c) { return &c->hash; }

#ifdef __cplusplus
}
#endif
