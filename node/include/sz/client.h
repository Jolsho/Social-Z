#pragma once
#include "sz/api/vec.h"
#include "sz/codec.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef NATIVE

//void some_networking_func();

#endif


/*
 *  TODO
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

#define TAG_LEN 16
typedef struct {
    Key             remote;
    Key             sym;
    Nonce           nonce;
    unsigned char   tag[TAG_LEN];
    unsigned char*  AD;
    size_t          AD_LEN;
}CryptCtx;

CryptCtx* new_ctx(UserCtx* user, uint8_t* head, size_t head_len);
bool decrypt(CryptCtx* ctx, uint8_t* buf, size_t len);
bool encrypt(CryptCtx* ctx, uint8_t* buf, size_t len);



#define OBJ_CARD 1
inline int obj_card() { return OBJ_CARD; }

#define OBJ_USER 2
inline int obj_user() { return OBJ_USER; }


// ITERATOR
typedef struct {
    Vec**   bufs;
    size_t  bufs_size;
    size_t  bufs_cap;

    size_t  buff_idx;
    size_t  item_idx;
    size_t  obj_size;

    size_t* sizes;
    size_t  size;
}Iterator;
Iterator* iterator_new(int obj_enum);
size_t iterator_remaining(Iterator* it);
static inline size_t iterator_size(Iterator* it) { return it->size; }
void* iterator_seek(Iterator* it, size_t idx);
void* iterator_next(Iterator* it);
void* iterator_prev(Iterator* it);


// CARDS
static inline Card* iterator_seek_card(Iterator* it, size_t idx) { return (Card*)iterator_seek(it, idx); }
static inline Card* iterator_next_card(Iterator* it) { return (Card*)iterator_next(it); }
static inline Card* iterator_prev_card(Iterator* it) { return (Card*)iterator_prev(it); }

static inline Card* card_new() { return malloc(sizeof(Card)); }
static inline void card_delete(Card* c) { free(c); }
static inline int card_get_id(const Card* c) { return c->id; }
static inline void card_set_id(Card* c, int id) { c->id = id; }
static inline int card_get_created_at(const Card* c) { return c->created_at; }
static inline void card_set_created_at(Card* c) { c->created_at = time(NULL); }
static inline HashT* card_get_hash(Card* c) { return &c->hash; }

// TODO --> How to do card hashing??

#ifdef __cplusplus
}
#endif
