/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma  once
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

/*
 *  The order is 
 *      create_context -> marshal -> ?encrypt -> send
 *      ?decrypt -> validate -> parse -> ?close
 *
 *  --------------------------------------------
 *
 *  marshal_next_feed_page_request(Client*, uint8_t*, size_t)
 *  parse_feed_page_response(Client*, uint8_t*, size_t)
 *
 *  marshal_next_feed_reipient_request(Client*, uint8_t*, size_t)
 *  parse_feed_recipient_response(Client*, uint8_t*, size_t)
 *
 *  marshal_blob_request(Client*, HashT*, uint8_t*, size_t)
 *  parse_blob_response(Client*, uint8_t*, size_t)
 *
 *
 *  --------------------------------------------
 *
 *  Connection_ID
 *      -> State
 *      -> Last_Used
 *      -> Parser
 */

typedef int (*Parser) (struct Client* cli, ContextID id, uint8_t* b, uint64_t l);

typedef struct __attribute__((packed)) Buffer {
    uint8_t*    b;
    uint32_t    cap;
    uint32_t    size;
}Buffer;

typedef struct __attribute__((packed)) ConState {
    uint8_t     state;
    uint8_t     parser_id;
} ConState;


typedef struct Client {
    ///////////// USER //////////////
    KeyPair     keys;
    Key         data_key;

    ///////// DATA STORES ///////////
    Feed        post_feed;
    Store       blob_store;


    /////////// BUFFERS /////////////
    BufferPool  pool;


    /////////// CONTEXTS ///////////

    /* IDs */
    ContextID*  ids;
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

} Client;
