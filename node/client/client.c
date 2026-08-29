/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "context.h"
#include <stdlib.h>


Client* init_client_state() {
    Client* cs = malloc(sizeof(Client));

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

    return cs;
}

ContextID client_new_context(Client* cli) {
    if (cli->ids_size <= 0) return -1;
    ContextID id = cli->ids[cli->ids_size - 1];
    cli->ids_size--;
    cli->states[id].state = CON_IDLE;
    return id;
}

void client_free_context(Client* cli, ContextID id) {
    if (cli->ids_size >= ID_CAP || !valid_id(id)) return;

    memset(&cli->states[id], 0, sizeof(ConState));
    cli->since_used_last[id] = 0;

    context_release_recv_buffer(cli, id);

    context_release_send_buffer(cli, id);

    cli->ids[cli->ids_size++] = id;
}

int client_parse_response(Client* cli, ContextID id, uint8_t* b, uint64_t l) {
    if (!valid_id(id)) return CLIENT_INVALID_ID;

     ConState state = cli->states[id];

    if (state.parser_id >= cli->parsers_count) return CLIENT_ERR;

    return cli->parsers[state.parser_id](cli, id, b, l);
}
