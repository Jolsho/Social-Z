/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "sz_client/client.h"
#include <stdlib.h>


struct Client* init_client_state() {
    struct Client* cs = malloc(sizeof(struct Client));

    if (buffer_pool_init(&cs->pool, 
        256,   1024,   // 1024 × 256 B  =  262,1144
        4096,  256,    // 256 × 4 KiB   =  1,048,576
        65536, 64      // 64 × 64 KiB   =  4,194,304
    ) != 0) {
        free(cs);
        return NULL;
    }

    #define BLOB_STORE_SIZE 1024 * 1024 * 25 // 25 MB
    if (store_setup(&cs->blob_store, BLOB_STORE_SIZE) != STORE_OK) {
        buffer_pool_destroy(&cs->pool);
        free(cs);
        return NULL;
    }

    memset(&cs->input, 0, sizeof(cs->input));

    return cs;
}

void start_client() {
    struct Client* cli = init_client_state();

    for (;;) {

        // TODO -> capture input and update cli->input

        client_update_state(cli);
        client_render_frame(cli);
    }
}

void client_update_state(struct Client* cli) {
    // TODO -> RUN SOME CHECKS TO SEE WHAT IS GOING ON HERE
    // like update the fucking state...
}

void client_render_frame(struct Client* client) {
    // TODO
}
