/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "context.h"

int context_release_recv_buffer(Client* cli, ContextID id) {
    if (!valid_id(id)) CLIENT_INVALID_ID;

    Buffer* b = &cli->recv_buffers[id];
    if (!b->b) return CLIENT_OK;

    int r = buffer_pool_push(&cli->pool, b->b, b->cap);
    if (r != 0) return CLIENT_ERR;

    memset(b, 0, sizeof(Buffer));
    return CLIENT_OK;
}

int context_release_send_buffer(Client* cli, ContextID id) {
    if (!valid_id(id)) CLIENT_INVALID_ID;

    Buffer* b = &cli->send_buffers[id];
    if (!b->b) return CLIENT_OK;

    int r = buffer_pool_push(&cli->pool, b->b, b->cap);
    if (r != 0) return CLIENT_ERR;

    memset(b, 0, sizeof(Buffer));
    return CLIENT_OK;
}

int buffer_ensure_min_cap(
    Client* cli, Buffer* buff, uint32_t minimum
) {

    if (buff->b && buff->cap < minimum) {
        if (buffer_pool_push(&cli->pool, buff->b, buff->cap) < 0)
            return CLIENT_ERR;
        buff->b = NULL;
        buff->cap = 0;
    }

    if (!buff->b) {
        size_t cap = minimum;

        buff->b = buffer_pool_pop(&cli->pool, &cap);
        if (!buff->b) return CLIENT_ERR;

        buff->cap = cap;
    }

    return CLIENT_OK;
}
