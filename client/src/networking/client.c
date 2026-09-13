/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "networking/context.h"

ContextID client_new_context(struct Client* cli) {
    if (cli->ids_size <= 0) return -1;
    ContextID id = cli->ids[cli->ids_size - 1];
    cli->ids_size--;
    cli->states[id].state = CON_IDLE;
    return id;
}

void client_free_context(struct Client* cli, ContextID id) {
    if (cli->ids_size >= ID_CAP || !valid_id(id)) return;

    memset(&cli->states[id], 0, sizeof(ConState));
    cli->since_used_last[id] = 0;

    context_release_recv_buffer(cli, id);

    context_release_send_buffer(cli, id);

    cli->ids[cli->ids_size++] = id;
}

int client_parse_response(struct Client* cli, ContextID id, uint8_t* b, uint64_t l) {
    if (!valid_id(id)) return CLIENT_INVALID_ID;

     ConState state = cli->states[id];

    if (state.parser_id >= cli->parsers_count) return CLIENT_ERR;

    return cli->parsers[state.parser_id](cli, id, b, l);
}

