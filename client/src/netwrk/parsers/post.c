/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_client/client.h"
#include "netwrk/context.h"
#include "netwrk/parsers.h"

int parse_post_feed_response(
    struct Client* cli, ContextID id,
    uint8_t* b, uint64_t len
) {

    if (len < 1 + POST_SIZE)
        return CLIENT_SMALL_BUFFER;

    bool has_more = (*b == 1);
    b++;

    if (feed_append_posts(&cli->post_feed, b, len) != FEED_OK)
        return CLIENT_ERR;

    return CLIENT_PARSE_DONE;
}
