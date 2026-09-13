/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "client.h"

#define PARSER_ID_USER_DATA 1
int parse_user_data(struct Client* cli, ContextID id, uint8_t* b, uint64_t len);

#define PARSER_ID_POST_FEED 2
int parse_post_feed_response(struct Client* cli, ContextID id, uint8_t* b, uint64_t len);

#define PARSER_ID_BLOB 3
int parse_blob(struct Client* cli, ContextID id, uint8_t* b, uint64_t len);


#define PARSER_ID_CAP 4

static const Parser parsers[PARSER_ID_CAP] = { 
    parse_user_data,
    parse_post_feed_response,

    NULL
};

