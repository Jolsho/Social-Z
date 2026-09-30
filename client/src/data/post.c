/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_client/client.h"
#include "netwrk/context.h"
#include "netwrk/parsers.h"

#define GET_POST_REQUEST_SIZE   KEY_SIZE + sizeof(int)

int marshal_get_post_request(
    struct Client* cli, ContextID id, int offset
) {
    if (!valid_id(id)) return CLIENT_INVALID_ID;


    ConState* s = &cli->net.states[id];
    if (s->state != CON_IDLE) 
        return CLIENT_CONN_BUSY;


    size_t cap = GET_POST_REQUEST_SIZE;
    uint8_t* bytes = buffer_pool_pop(&cli->pool, &cap);
    Buffer buff = {
        .b = bytes,
        .cap = cap,
        .size = 0,
    };


    memcpy(buff.b, cli->keys.pub.b, KEY_SIZE);
    buff.b += KEY_SIZE;
    buff.size += KEY_SIZE;

    memcpy(buff.b, &offset, sizeof(int));
    buff.b += sizeof(int);
    buff.size += sizeof(int);

    s->parser_id = PARSER_ID_POST_FEED;
    s->state = CON_SENDING;

    // TODO => append to send buffers.

    return CLIENT_OK;
}

int parse_post_feed_response(
    struct Client* cli, ContextID id, 
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
