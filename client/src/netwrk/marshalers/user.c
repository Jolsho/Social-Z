/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "netwrk/context.h"
#include "netwrk/marshalers.h"
#include "netwrk/parsers.h"

int marshal_get_user_data_request(struct Client* cli, ContextID id) {
    if (!valid_id(id))
        return CLIENT_INVALID_ID;
    if (!cli || cli->login.id != id)
        return CLIENT_ERR;

    ConState* state = &cli->net.states[id];
    if (state->state != CON_IDLE || state->send_owned)
        return CLIENT_CONN_BUSY;

    Buffer* request = &cli->net.send_buffers[id];
    uint32_t size = cli->login.lookup_ready
        ? LOGIN_FETCH_SIZE
        : 6 + login_username_size(cli->login.username);

    if (buffer_ensure_min_cap(cli, request, size) != CLIENT_OK)
        return CLIENT_ERR;

    if (cli->login.lookup_ready) {
        login_fetch_request(request->b, &cli->login.lookup.hash);
        cli->login.fetching = true;
    } else {
        login_lookup_request(request->b, cli->login.username);
    }

    request->size = size;
    state->parser_id = PARSER_ID_USER_DATA;
    state->state = CON_SENDING;

    return CLIENT_OK;
}
