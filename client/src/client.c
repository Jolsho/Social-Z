/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "sz_client/client.h"
#include "networking/dispatch.h"
#include <stdlib.h>


struct Client* init_client(void) {
    struct Client* cs = calloc(1, sizeof(*cs));
    if (!cs) return NULL;
    cs->wrld.bvh.root = cs->wrld.bvh.free_list = BVH_NULL;
    cs->wrld.focused = ENTITY_ID_INVALID;
    // Framed 64 KiB chunks need 21 extra bytes; round up to keep pool entries aligned.
    if (buffer_pool_init(&cs->pool, 256, 1024, 4096, 256, 65536 + 32, 64) != 0 ||
        store_setup(&cs->blob_store, 25 * 1024 * 1024) != STORE_OK ||
        init_networker(&cs->net) != CLIENT_OK) {
        destroy_client(cs);
        return NULL;
    }
    cs->net.parsers = parsers;
    cs->net.parsers_count = PARSER_ID_CAP;
    return cs;
}

void destroy_client(struct Client* cli) {
    if (!cli) return;
    destroy_networker(cli);
    store_destroy(&cli->blob_store);
    buffer_pool_destroy(&cli->pool);
    feed_destroy(&cli->feed);
    free(cli->wrld.bvh.nodes);
    free(cli->wrld.bvh.entity_to_leaf);
    volatile uint8_t* bytes = (volatile uint8_t*)cli;
    for (size_t i = 0; i < sizeof(*cli); i++) bytes[i] = 0;
    free(cli);
}
