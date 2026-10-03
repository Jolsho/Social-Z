/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_client/client.h"
#include "utils/buffers.h"

typedef int (*Parser) (struct Client* cli, ContextID id, uint8_t* b, uint64_t l);
typedef void (*ParserCleanup)(struct Client* cli, ContextID id);

typedef struct ParserEntry {
    Parser parse;
    // Runs once at context close, before networking buffers are released.
    // Release operation resources here; do not free the context recursively.
    ParserCleanup cleanup;
} ParserEntry;

typedef struct __attribute__((packed)) {
    uint8_t     state;
    uint8_t     parser_id;
    ContextID   next_free;
    bool        send_owned, release_pending;

    // Client-selected state; the registered cleanup handler releases any owned resources.
    void*       operation;

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
    const ParserEntry* parsers;
    uint16_t    parsers_count;

} Networker;


// Initializes fresh state; destroy it before initializing it again.
int init_networker(Networker* net);
void destroy_networker(struct Client* cli);
ContextID client_new_context(Networker* net);
void client_free_context(struct Client* cli, ContextID id);
