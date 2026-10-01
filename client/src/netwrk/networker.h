/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_client/client.h"
#include "utils/buffers.h"
#include "sz_common/codec.h"

typedef int (*Parser) (struct Client* cli, ContextID id, uint8_t* b, uint64_t l);

typedef struct __attribute__((packed)) {
    uint8_t     state;
    uint8_t     parser_id;
    ContextID   next_free;
    bool        blob_active;
    HashT       blob_hash;

#ifndef PLATFORM_WASM
    int         fd;
#endif

} ConState;


#define MAX_CONNS 128

typedef struct {

    /* IDs */
    ContextID   free_head; // ;)

    /* Components */
    ConState    states[MAX_CONNS];
    uint32_t    since_used_last[MAX_CONNS];

    /* Buffers */
    Buffer*     recv_buffers;
    Buffer*     send_buffers;

    /* Parsers */
    const Parser* parsers;
    uint16_t    parsers_count;

} Networker;


// Initializes fresh state; destroy it before initializing it again.
int init_networker(Networker* net);
void destroy_networker(struct Client* cli);
ContextID client_new_context(Networker* net);
void client_free_context(struct Client* cli, ContextID id);
