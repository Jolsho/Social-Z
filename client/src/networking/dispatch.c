/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "networking/dispatch.h"
#include "operations/blob.h"
#include "operations/login.h"
#include "operations/package.h"
#include "operations/feed_page.h"

const HandlerEntry handlers[HANDLER_ID_CAP] = {
    [HANDLER_ID_USER_DATA] = {
        .parse_response = login_handle_response,
        .cleanup = login_cleanup,
    },
    [HANDLER_ID_BLOB] = {
        .parse_response = blob_handle_response,
        .cleanup = blob_cleanup,
    },
    [HANDLER_ID_PACKAGE] = {
        .parse_response = package_handle_response,
        .cleanup = package_cleanup,
    },
    [HANDLER_ID_FEED_PAGE] = {
        .parse_response = feed_page_handle_response,
        .cleanup = feed_page_cleanup,
    },
};
