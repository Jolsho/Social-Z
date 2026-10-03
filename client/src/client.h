/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma  once
#include "wrld/wrld.h"
#include "networking/networker.h"
#include "utils/store.h"
#include "content/feed.h"
#include "codec/account.h"

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
    AccountHeader   account;
    bool            logged_in;
    ContextID       login_id;

    ///////// DATA STORES ///////////
    Feed            feed;
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

inline void send_request(struct Client* cli, ContextID id, struct Buffer* b) {

    // TODO --> this is very naive. You need to enqueue these things.

    ssize_t n = send(cli->net.states[id].fd, b->b, b->size, 0);
    bool failed = n < 0 || (size_t)n != b->size;
    client_return_buffer(cli, b);
    if (failed) client_cancel_login(cli);
}
#endif
