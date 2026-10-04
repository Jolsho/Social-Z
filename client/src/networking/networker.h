/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_client/client.h"
#include "utils/buffers.h"

typedef int (*ResponseParser)(
    struct Client* cli, ContextID id, uint8_t* bytes, uint64_t size
);

// Advance one outgoing step, independently of response parsing.
// Return CLIENT_OK; stop sending when the request body is complete.
// An error lets networking cancel and clean the context.
typedef int (*Sender)(struct Client* cli, ContextID id);
typedef void (*ParseStateCleanup)(struct Client* cli, ContextID id);

typedef struct HandlerEntry {
    ResponseParser parse_response;

    // Runs once at context close, before networking buffers are released.
    // Release context resources here; do not free the context recursively.
    ParseStateCleanup cleanup;

    Sender sender; // Optional; response-only operations leave this NULL.
} HandlerEntry;

typedef struct __attribute__((packed)) {
    uint8_t     state;
    uint8_t     handler_id;
    ContextID   next_free;
    ContextID   next_sender; // Single list of outgoing contexts; zero ends the list.
    bool        send_owned, release_pending;
    bool        sending;
    bool        request_started; // The host accepted begin; cancellation must abort its transport.

    // Shared state for response parsing and sending; cleanup releases its owned resources.
    void*       context;

#ifndef PLATFORM_WASM
    int         fd;
#endif

} ConState;


#define MAX_CONNS 128

typedef struct {

    BufferPool* pool; // Borrowed; outlives networking and all retained send buffers.
    struct Client* client; // Owner supplied to operation handlers and host hooks.

    /* IDs */
    ContextID   free_head; // ;)
    ContextID   send_head, send_cursor;
    bool        polling; // Prevent host callbacks from polling recursively.

    /* Components */
    ConState    states[MAX_CONNS];
    uint32_t    since_used_last[MAX_CONNS];

    /* Buffers */
    Buffer*     recv_buffers;
    Buffer*     send_buffers;

    /* Handlers */
    const HandlerEntry* handlers;
    uint16_t    handlers_count;

} Networker;


// Initializes fresh state; destroy it before initializing it again.
// Set pool, client, and handlers before starting requests.
int init_networker(Networker* net);
void destroy_networker(Networker* net);
ContextID networker_new_context(Networker* net);
void networker_free_context(Networker* net, ContextID id);

int networker_parse_response(
    Networker* net, ContextID id, uint8_t* bytes, uint64_t size
);
void networker_return_buffer(Networker* net, Buffer* buffer);
int networker_cancel_request(Networker* net, ContextID id);
int networker_request_failed(Networker* net, ContextID id);

// Register once after installing the handler and its context state.
// Buffer waits stay on the list; each poll checks just one context.
void networker_start_sending(Networker* net, ContextID id);
// Remove when the body is complete, or before context cleanup/reuse.
void networker_stop_sending(Networker* net, ContextID id);

// Check one sender and advance the cursor, even when its buffer is held.
bool networker_poll(Networker* net);
