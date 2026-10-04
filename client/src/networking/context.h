/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_client/client.h"
#include "utils/buffers.h"
#include "networking/networker.h"

#define ID_CAP MAX_CONNS

#define CON_DEAD       ((uint8_t) 0 )
#define CON_IDLE       ((uint8_t) 1 )
#define CON_SENDING    ((uint8_t) 2 )
#define CON_RECEIVING  ((uint8_t) 3 )

static inline bool valid_id(ContextID id) {
    return (id < ID_CAP && id > 0);
}
int context_release_send_buffer(Networker* net, ContextID id);
int context_release_recv_buffer(Networker* net, ContextID id);

int buffer_ensure_min_cap(BufferPool* pool, Buffer* buff, uint32_t min);

// Submit an already marshaled, complete request through the host lifecycle.
// Streaming operations will use the same hooks across multiple polls.
int context_send_request(Networker* net, ContextID id);
