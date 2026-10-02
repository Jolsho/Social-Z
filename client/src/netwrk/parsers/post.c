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

    // TODO: Replace this disabled legacy parser with verified feed-page retrieval.
    (void)cli;
    (void)id;
    (void)b;
    (void)len;
    return CLIENT_ERR;
}
