/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma  once
#include "wrld/wrld.h"
#include "netwrk/networker.h"
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


struct Client {
    ///////////// USER //////////////
    KeyPair         keys;
    Key             data_key;

    ///////// DATA STORES ///////////
    Feed            post_feed;
    Store           blob_store;


    /////////// BUFFERS /////////////
    BufferPool      pool;


    /////////// NETWORKING /////////////
    Networker       net;


    /////////// ECS WORLD STATE /////////////
    World           wrld;
};

#if defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
#include "sys/socket.h"

inline void send_request(Client* cli, ContextID id, Buffer* b) {

    // TODO --> this is very naive. You need to enqueue these things.

    uint64_t n = send(cli->states[id].fd, b->b, b->size, 0);
    buffer_pool_push(&cli->pool, b->b, b->cap);
}
#endif
