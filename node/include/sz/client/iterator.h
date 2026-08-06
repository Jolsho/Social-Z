/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz/api/vec.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef NATIVE

//void some_networking_func();

#endif

/*
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


#ifdef __cplusplus
}
#endif
