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
    bool        blob_active;
    HashT       blob_hash;

#ifndef PLATFORM_WASM
    int         fd;
#endif

} ConState;


struct __attribute__((packed)) DeadConn {
    int16_t id;
    int16_t next;
};


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
    Parser*     parsers;
    uint16_t    parsers_count;

} Networker;


inline void init_networker(Networker* net) {


}
