/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "networking/context.h"
#include <string.h>
#include "networking/networker.h"
#include <stdlib.h>

int init_networker(Networker* net) {
    if (!net) {
        return CLIENT_ERR;
    }
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

void destroy_networker(Networker* net) {
    if (!net) {
        return;
    }
    for (ContextID id = 1; id < MAX_CONNS; id++) {
        if (net->states[id].state != CON_DEAD) {
            networker_cancel_request(net, id);
        }
    }
    free(net->recv_buffers);
    free(net->send_buffers);
    memset(net, 0, sizeof(*net));
    net->free_head = INT16_MAX;
}

ContextID networker_new_context(Networker* net) {
    if (!net || !net->recv_buffers || !net->send_buffers ||
        !valid_id(net->free_head)) {
        return -1;
    }
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

void networker_free_context(Networker* net, ContextID id) {
    if (!net || !valid_id(id)) {
        return;
    }

    ConState* state = &net->states[id];
    if (state->state == CON_DEAD && !state->release_pending) {
        return;
    }

    if (state->state != CON_DEAD) {
        networker_stop_sending(net, id);
        // Cleanup runs once, even if the context must wait for the host's send buffer.
        state->state = CON_DEAD;
        if (net->handlers && state->handler_id < net->handlers_count) {
            ParseStateCleanup cleanup = net->handlers[state->handler_id].cleanup;
            if (cleanup) {
                cleanup(net->client, id);
            }
        }
        state->context = NULL;
    }

    context_release_recv_buffer(net, id);

    // The host may still be sending. Keep this context reserved until it returns the buffer.
    if (state->send_owned) {
        state->release_pending = true;
        return;
    }
    context_release_send_buffer(net, id);

    net->since_used_last[id] = 0;

    memset(state, 0, sizeof(*state));
    state->next_free = net->free_head;
    net->free_head = id;
}

int networker_parse_response(
    Networker* net, ContextID id, uint8_t* bytes, uint64_t size
) {
    if (!valid_id(id)) {
        return CLIENT_INVALID_ID;
    }
    if (!net || !net->handlers) {
        return CLIENT_ERR;
    }

    const ConState* state = &net->states[id];
    if (state->state == CON_DEAD || state->handler_id >= net->handlers_count ||
        !net->handlers[state->handler_id].parse_response) {
        return CLIENT_ERR;
    }

    return net->handlers[state->handler_id].parse_response(net->client, id, bytes, size);
}

void networker_return_buffer(Networker* net, struct Buffer* buff) {
    if (!net || !buff || !net->send_buffers) {
        return;
    }

    for (ContextID id = 1; id < MAX_CONNS; id++) {
        ConState* state = &net->states[id];
        if (buff != &net->send_buffers[id] || !state->send_owned) {
            continue;
        }

        state->send_owned = false;
        context_release_send_buffer(net, id);

        if (state->release_pending) {
            networker_free_context(net, id);
        }
        return;
    }
}

int networker_cancel_request(Networker* net, ContextID id) {
    if (!valid_id(id)) {
        return CLIENT_INVALID_ID;
    }
    if (!net || net->states[id].state == CON_DEAD) {
        return CLIENT_ERR;
    }

    // Abort may release a host-held buffer, but cannot reenter response dispatch.
    if (net->states[id].request_started) {
        request_abort(net->client, id);
    }
    networker_free_context(net, id);
    return CLIENT_OK;
}

int networker_request_failed(Networker* net, ContextID id) {
    if (!valid_id(id)) {
        return CLIENT_INVALID_ID;
    }
    if (!net || net->states[id].state == CON_DEAD) {
        return CLIENT_ERR;
    }

    networker_free_context(net, id);
    return CLIENT_ERR;
}
