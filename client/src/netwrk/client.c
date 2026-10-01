/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "netwrk/context.h"
#include "netwrk/networker.h"
#include <stdlib.h>

int init_networker(Networker* net) {
    if (!net) return CLIENT_ERR;
    memset(net, 0, sizeof(*net));
    net->free_head = INT16_MAX;
    net->recv_buffers = calloc(MAX_CONNS, sizeof(Buffer));
    net->send_buffers = calloc(MAX_CONNS, sizeof(Buffer));
    if (!net->recv_buffers || !net->send_buffers) {
        free(net->recv_buffers);
        free(net->send_buffers);
        net->recv_buffers = net->send_buffers = NULL;
        return CLIENT_ERR;
    }
    for (ContextID id = 1; id < MAX_CONNS; id++) {
        net->states[id].next_free = id + 1;
#ifndef PLATFORM_WASM
        net->states[id].fd = -1;
#endif
    }
    net->states[MAX_CONNS - 1].next_free = INT16_MAX;
    net->free_head = 1;
    return CLIENT_OK;
}

void destroy_networker(struct Client* cli) {
    if (!cli) return;
    for (ContextID id = 1; id < MAX_CONNS; id++) client_free_context(cli, id);
    free(cli->net.recv_buffers);
    free(cli->net.send_buffers);
    memset(&cli->net, 0, sizeof(cli->net));
    cli->net.free_head = INT16_MAX;
}

ContextID client_new_context(Networker* net) {
    if (!net || !net->recv_buffers || !net->send_buffers || !valid_id(net->free_head)) return -1;
    ContextID id = net->free_head;
    ConState* c = &net->states[id];
    net->free_head = c->next_free;
    memset(c, 0, sizeof(ConState));
    c->state = CON_IDLE;
#ifndef PLATFORM_WASM
    c->fd = -1;
#endif
    return id;
}

void client_free_context(struct Client* cli, ContextID id) {
    if (!cli || !valid_id(id)) return;
    Networker* net = &cli->net;
    if (net->states[id].state == CON_DEAD && !net->states[id].release_pending) return;
    if (cli->login.id == id) {
        volatile uint8_t* secret = cli->login.password.b;
        for (size_t i = 0; i < cli->login.password.cap; i++) secret[i] = 0;
        if (secret) buffer_pool_push(&cli->pool, cli->login.password.b, cli->login.password.cap);
        memset(&cli->login, 0, sizeof(cli->login));
    }

    if (net->states[id].blob_active) {
        HashT hash = net->states[id].blob_hash;
        StoreItem* item = ht_lookup(&cli->blob_store.table, &hash);
        if (item && item->context == id && item->received < item->size)
            store_erase_item(&cli->blob_store, &hash);
    }
    context_release_recv_buffer(cli, id);

    // The host may still be sending. Keep this context reserved until it returns the buffer.
    if (net->states[id].send_owned) {
        net->states[id].release_pending = true;
        net->states[id].state = CON_DEAD;
        return;
    }
    context_release_send_buffer(cli, id);

    net->since_used_last[id] = 0;

    ConState* c = &net->states[id];
    memset(c, 0, sizeof(ConState));

    c->next_free = net->free_head;

    net->free_head = id;
}

int client_parse_response(struct Client* cli, ContextID id, uint8_t* b, uint64_t l) {
    if (!valid_id(id)) return CLIENT_INVALID_ID;
    if (!cli || !cli->net.parsers) return CLIENT_ERR;
    ConState state = cli->net.states[id];
    if (state.state == CON_DEAD || state.parser_id >= cli->net.parsers_count ||
        !cli->net.parsers[state.parser_id]) return CLIENT_ERR;

    return cli->net.parsers[state.parser_id](cli, id, b, l);
}
