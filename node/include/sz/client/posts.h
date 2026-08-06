
/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz/client/client.h"
#include "sz/client/iterator.h"
#include "sz/codec.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef NATIVE

//void some_networking_func();

#endif

bool marshal_request(UserCtx* ctx, void* r, size_t r_size, uint8_t* buff, size_t len);

typedef struct {
    int page;
    bool pending;
} GetPostRequest;
static inline void get_posts_req_set_page(GetPostRequest* r, int page) { r->page = page; }
static inline void get_posts_req_set_pending(GetPostRequest* r, bool is_pending) { r->pending = is_pending; }


// Posts
static inline Post* iterator_seek_post(Iterator* it, size_t idx) { return (Post*)iterator_seek(it, idx); }
static inline Post* iterator_next_post(Iterator* it) { return (Post*)iterator_next(it); }
static inline Post* iterator_prev_post(Iterator* it) { return (Post*)iterator_prev(it); }


static inline Post* post_new() { return malloc(sizeof(Post)); }
static inline void post_delete(Post* c) { free(c); }
static inline int post_get_id(const Post* c) { return c->id; }
static inline void post_set_id(Post* c, int id) { c->id = id; }
static inline const char* post_get_created_at(const Post* c) { return c->created_at; }
char* post_set_created_at(Post* c);
static inline size_t post_created_at_len() { return 8; }
static inline HashT* post_get_hash(Post* c) { return &c->hash; }


#ifdef __cplusplus
}
#endif
