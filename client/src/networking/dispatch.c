/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "networking/dispatch.h"
#include "operations/blob.h"
#include "operations/login.h"
#include "operations/package.h"

const ParserEntry parsers[PARSER_ID_CAP] = {
    [PARSER_ID_USER_DATA] = {login_handle_response, login_cleanup},
    [PARSER_ID_BLOB] = {blob_handle_response, blob_cleanup},
    [PARSER_ID_PACKAGE] = {package_handle_response, package_cleanup},
};
