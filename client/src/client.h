/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma  once
#include "input/input.h"
#include "sz_client/client.h"
#include "utils/store.h"
#include "data/feed.h"

#ifdef NATIVE

//#include "net/networker.h"
    /*
     *  TODO
     *      Also we are going to need connection manager and shit for native.
     *          Like progress trackers and shit. (offsets in store copying).
     *  So we read in buffers from network.
     *  We then parse out crypto stuff and decrypt.
     *      (keys / ID & nonce & stuff) then decrypt chunk.
     *  We then copy then parse the decrypted body.
     *  And copy what we want into another buffer which can be stored.
    */

#endif


typedef int (*Parser) (struct Client* cli, ContextID id, uint8_t* b, uint64_t l);

typedef struct __attribute__((packed)) {
    uint8_t*    b;
    uint32_t    cap;
    uint32_t    size;
}Buffer;

typedef struct __attribute__((packed)) {
    uint8_t     state;
    uint8_t     parser_id;

#ifndef PLATFORM_WASM
    int         fd;
#endif

} ConState;


struct Client {
    ///////////// USER //////////////
    KeyPair     keys;
    Key         data_key;

    ///////// DATA STORES ///////////
    Feed        post_feed;
    Store       blob_store;


    /////////// BUFFERS /////////////
    BufferPool  pool;


    /////////// NETWORKING ///////////

    /* IDs */
    ContextID*  ids; // TODO -> REUSE ConState for freeLIST
    uint16_t    ids_size;

    /* Components */
    ConState*   states;
    uint32_t*   since_used_last;

    /* Buffers */
    Buffer*     recv_buffers;
    Buffer*     send_buffers;

    /* Parsers */
    Parser*     parsers;
    uint16_t    parsers_count;

    ///////////////////////////////

    InputState  input;

};

#if defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
#include "sys/socket.h"

inline void send_request(Client* cli, ContextID id, Buffer* b) {

    // TODO --> this is very naive. You need to enqueue these things.

    uint64_t n = send(cli->states[id].fd, b->b, b->size, 0);
    buffer_pool_push(&cli->pool, b->b, b->cap);
}
#endif
