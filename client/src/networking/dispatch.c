/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "networking/dispatch.h"
#include "codec/blob.h"
#include "operations/login.h"

// The legacy post-feed response is disabled until feed-page retrieval replaces it.
const Parser parsers[PARSER_ID_CAP] = {
    [PARSER_ID_USER_DATA] = login_handle_response,
    [PARSER_ID_BLOB] = parse_blob,
};
