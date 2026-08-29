/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_client/client.h"
#include "client.h"

#define CON_DEAD       ((uint8_t) 0 )
#define CON_IDLE       ((uint8_t) 1 )
#define CON_SENDING    ((uint8_t) 2 )
#define CON_RECEIVING  ((uint8_t) 3 )

inline bool valid_id(ContextID id) {
    return (id < ID_CAP && id > 0);
}
inline void context_set_state(Client* cli, ContextID id, uint8_t new_state) {
    if (!valid_id(id) || new_state > CON_RECEIVING) return;
    cli->states[id].state = new_state;
}
inline Buffer* context_get_recv_buffer(Client* cli, ContextID id) {
    if (!valid_id(id)) return NULL;
    return &cli->recv_buffers[id];
}
inline Buffer* context_get_send_buffer(Client* cli, ContextID id) {
    if (!valid_id(id)) return NULL;
    return &cli->send_buffers[id];
}

int context_release_send_buffer(Client* cli, ContextID id);
int context_release_recv_buffer(Client* cli, ContextID id);

int buffer_ensure_min_cap(Client* cli, Buffer* buff, uint32_t min);
