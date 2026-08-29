/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_node/utils/error.h"
#include <string.h>
#include <stdlib.h>

int marshal_error(
    Error* e, 
    Msg* msg, 
    Actors too
) {
    msg->code = e->code;
    msg->id = e->id;
    msg->too = too;
    msg->is_wiped = false;

    size_t msg_size = strlen(e->msg);
    if (!msg->data || msg->data->cap < msg_size) {
        return -1;
    }
    Vec* d = msg->data;

    vec_write(d, e->key.b, KEY_SIZE);
    vec_write(d, &e->r, sizeof(int));
    vec_write(d, &msg_size, sizeof(size_t));
    vec_write(d, e->msg, msg_size);

    return 0;
}

void unmarshal_error(Error* e, Msg* m) {
    e->code = m->code;
    e->id = m->id;

    if (!m->data) return;
    Vec* d = m->data;

    vec_read(d, e->key.b, KEY_SIZE);
    vec_read(d, &e->r, sizeof(int));

    size_t len;
    vec_read(d, &len, sizeof(size_t));
    if (len < vec_remaining(d)) {
        if (e->msg && len > strlen(e->msg)) {
            free((uint8_t*)e->msg);
            e->msg = (char*)malloc(len);
        }
        vec_read(d, (uint8_t*)e->msg, len);
    }
}
