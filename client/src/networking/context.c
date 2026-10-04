/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "networking/context.h"
#include <string.h>

int context_release_recv_buffer(Networker* net, ContextID id) {
    if (!valid_id(id)) {
        return CLIENT_INVALID_ID;
    }

    Buffer* b = &net->recv_buffers[id];
    if (!b->b) {
        return CLIENT_OK;
    }

    int r = buffer_pool_push(net->pool, b->b, b->cap);
    if (r != 0) {
        return CLIENT_ERR;
    }

    memset(b, 0, sizeof(Buffer));
    return CLIENT_OK;
}

int context_release_send_buffer(Networker* net, ContextID id) {
    if (!valid_id(id)) {
        return CLIENT_INVALID_ID;
    }

    if (net->states[id].send_owned) {
        return CLIENT_CONN_BUSY;
    }
    Buffer* b = &net->send_buffers[id];
    if (!b->b) {
        return CLIENT_OK;
    }

    int r = buffer_pool_push(net->pool, b->b, b->cap);
    if (r != 0) {
        return CLIENT_ERR;
    }

    memset(b, 0, sizeof(Buffer));
    return CLIENT_OK;
}

int buffer_ensure_min_cap(
    BufferPool* pool, Buffer* buff, uint32_t minimum
) {

    if (buff->b && buff->cap < minimum) {
        if (buffer_pool_push(pool, buff->b, buff->cap) < 0) {
            return CLIENT_ERR;
        }
        buff->b = NULL;
        buff->cap = 0;
    }

    if (!buff->b) {
        size_t cap = minimum;

        buff->b = buffer_pool_pop(pool, &cap);
        if (!buff->b) {
            return CLIENT_ERR;
        }

        buff->cap = cap;
    }

    return CLIENT_OK;
}

int context_send_request(Networker* net, ContextID id) {
    ConState* state = &net->states[id];
    state->state = CON_RECEIVING;
    if (request_begin(net->client, id) != CLIENT_OK) {
        networker_free_context(net, id);
        return CLIENT_ERR;
    }

    state->request_started = true;

    // Install ownership before write: the host may return the buffer immediately.
    state->send_owned = true;
    if (request_write(net->client, id, &net->send_buffers[id]) != CLIENT_OK) {
        state->send_owned = false;
        networker_cancel_request(net, id);
        return CLIENT_ERR;
    }

    // No responses until end, so returning a write buffer cannot recycle this context.
    if (request_end(net->client, id) != CLIENT_OK) {
        networker_cancel_request(net, id);
        return CLIENT_ERR;
    }
    return CLIENT_OK;
}
