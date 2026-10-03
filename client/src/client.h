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
