/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "netwrk/context.h"
#include "netwrk/networker.h"

ContextID client_new_context(Networker* net) {
    if (!net->free_head) return -1;


    ConState* c = &net->states[net->free_head];
    struct DeadConn dc = *(struct DeadConn*)c;

    memset(c, 0, sizeof(ConState));

    if (dc.next < ID_CAP)
        net->free_head = dc.next;
    else 
        net->free_head = INT16_MAX;

    net->states[dc.id].state = CON_IDLE;

    return dc.id;
}

void client_free_context(struct Client* cli, ContextID id) {
    Networker* net = &cli->net;
    if (!valid_id(id)) return;

    context_release_recv_buffer(cli, id);

    context_release_send_buffer(cli, id);

    net->since_used_last[id] = 0;

    ConState* c = &net->states[id];
    memset(c, 0, sizeof(ConState));

    struct DeadConn* dc = (struct DeadConn*)c;

    dc->id = id;
    dc->next = net->free_head;

    net->free_head = id;
}

int client_parse_response(struct Client* cli, ContextID id, uint8_t* b, uint64_t l) {
    if (!valid_id(id)) return CLIENT_INVALID_ID;

     ConState state = cli->net.states[id];

    if (state.parser_id >= cli->net.parsers_count) return CLIENT_ERR;

    return cli->net.parsers[state.parser_id](cli, id, b, l);
}

