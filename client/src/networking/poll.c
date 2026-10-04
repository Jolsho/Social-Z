/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "networking/context.h"

void networker_start_sending(Networker* net, ContextID id) {
    ConState* state = &net->states[id];
    if (state->sending || state->state == CON_DEAD ||
        state->handler_id >= net->handlers_count ||
        !net->handlers[state->handler_id].sender) {
        return;
    }

    state->sending = true;
    state->next_sender = net->send_head;
    net->send_head = id;
    if (!net->send_cursor) {
        net->send_cursor = id;
    }
}

void networker_stop_sending(Networker* net, ContextID id) {
    ConState* state = &net->states[id];
    if (!state->sending) {
        return;
    }

    ContextID previous = 0;
    for (ContextID current = net->send_head; current != id;
         current = net->states[current].next_sender) {
        previous = current;
    }

    if (previous) {
        net->states[previous].next_sender = state->next_sender;
    } else {
        net->send_head = state->next_sender;
    }
    if (net->send_cursor == id) {
        net->send_cursor = state->next_sender ? state->next_sender : net->send_head;
    }
    state->sending = false;
    state->next_sender = 0;
}

bool networker_poll(Networker* net) {
    if (net->polling || !net->send_cursor) {
        return net->send_head != 0;
    }

    ContextID id = net->send_cursor;
    ConState* state = &net->states[id];
    // Advance before calling the sender: it may complete or cancel contexts synchronously.
    net->send_cursor = state->next_sender ? state->next_sender : net->send_head;
    net->polling = true;

    if (!state->send_owned) {
        Sender sender = net->handlers[state->handler_id].sender;
        if (sender(net->client, id) != CLIENT_OK) {
            networker_cancel_request(net, id);
        }
    }

    net->polling = false;
    return net->send_head != 0;
}
