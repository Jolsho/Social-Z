/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "networking/context.h"
#include "networking/networker.h"
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
    for (ContextID id = 1; id < MAX_CONNS; id++) {
        if (cli->net.states[id].state != CON_DEAD) {
            client_cancel_request(cli, id);
        }
    }
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
    if (!cli || !valid_id(id)) {
        return;
    }

    Networker* net = &cli->net;
    ConState* state = &net->states[id];
    if (state->state == CON_DEAD && !state->release_pending) {
        return;
    }

    if (state->state != CON_DEAD) {
        // Cleanup runs once, even if the context must wait for the host's send buffer.
        state->state = CON_DEAD;
        if (net->parsers && state->parser_id < net->parsers_count) {
            ParseStateCleanup cleanup = net->parsers[state->parser_id].cleanup;
            if (cleanup) {
                cleanup(cli, id);
            }
        }
        state->context = NULL;
    }

    context_release_recv_buffer(cli, id);

    // The host may still be sending. Keep this context reserved until it returns the buffer.
    if (state->send_owned) {
        state->release_pending = true;
        return;
    }
    context_release_send_buffer(cli, id);

    net->since_used_last[id] = 0;

    memset(state, 0, sizeof(*state));
    state->next_free = net->free_head;
    net->free_head = id;
}

int client_parse_response(struct Client* cli, ContextID id, uint8_t* b, uint64_t l) {
    if (!valid_id(id)) {
        return CLIENT_INVALID_ID;
    }
    if (!cli || !cli->net.parsers) {
        return CLIENT_ERR;
    }

    const ConState* state = &cli->net.states[id];
    if (state->state == CON_DEAD || state->parser_id >= cli->net.parsers_count ||
        !cli->net.parsers[state->parser_id].parse_response) {
        return CLIENT_ERR;
    }

    return cli->net.parsers[state->parser_id].parse_response(cli, id, b, l);
}

void client_return_buffer(struct Client* cli, struct Buffer* buff) {
    if (!cli || !buff || !cli->net.send_buffers) {
        return;
    }

    for (ContextID id = 1; id < MAX_CONNS; id++) {
        ConState* state = &cli->net.states[id];
        if (buff != &cli->net.send_buffers[id] || !state->send_owned) {
            continue;
        }

        state->send_owned = false;
        context_release_send_buffer(cli, id);

        if (state->release_pending) {
            client_free_context(cli, id);
        }
        return;
    }
}

int client_cancel_request(struct Client* cli, ContextID id) {
    if (!valid_id(id)) {
        return CLIENT_INVALID_ID;
    }
    if (!cli || cli->net.states[id].state == CON_DEAD) {
        return CLIENT_ERR;
    }

    // Abort may release a host-held buffer, but cannot reenter response dispatch.
    if (cli->net.states[id].request_started) {
        request_abort(cli, id);
    }
    client_free_context(cli, id);
    return CLIENT_OK;
}

int client_request_failed(struct Client* cli, ContextID id) {
    if (!valid_id(id)) {
        return CLIENT_INVALID_ID;
    }
    if (!cli || cli->net.states[id].state == CON_DEAD) {
        return CLIENT_ERR;
    }

    client_free_context(cli, id);
    return CLIENT_ERR;
}
