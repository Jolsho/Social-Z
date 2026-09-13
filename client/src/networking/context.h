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
int context_release_send_buffer(struct Client* cli, ContextID id);
int context_release_recv_buffer(struct Client* cli, ContextID id);

int buffer_ensure_min_cap(struct Client* cli, Buffer* buff, uint32_t min);
