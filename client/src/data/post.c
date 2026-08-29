/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_client/client.h"
#include "context.h"
#include "parsers.h"

#define GET_POST_REQUEST_SIZE   KEY_SIZE + sizeof(int)

int marshal_get_post_request(
    Client* cli, ContextID id, int offset
) {
    if (!valid_id(id)) return CLIENT_INVALID_ID;


    ConState* s = &cli->states[id];
    if (s->state != CON_IDLE) 
        return CLIENT_CONN_BUSY;


    Buffer* buff = &cli->send_buffers[id];
    if (buffer_ensure_min_cap(cli, buff, GET_POST_REQUEST_SIZE) != CLIENT_OK)
        return CLIENT_ERR;
    uint8_t* b = buff->b;


    memcpy(b, cli->keys.pub.b, KEY_SIZE);
    b += KEY_SIZE;
    buff->size += KEY_SIZE;

    memcpy(b, &offset, sizeof(int));
    b += sizeof(int);
    buff->size += sizeof(int);

    s->parser_id = PARSER_ID_POST_FEED;
    s->state = CON_SENDING;

    return CLIENT_OK;
}

int parse_post_feed_response(
    Client* cli, ContextID id, 
    uint8_t* b, uint64_t len
) {

    if (len < 1 + MINIMUM_POST_SIZE)
        return CLIENT_SMALL_BUFFER;

    bool has_more = (*b == 1);
    b++;

    if (feed_append_posts(&cli->post_feed, b, len) != FEED_OK)
        return CLIENT_ERR;

    return CLIENT_PARSE_DONE;
}
