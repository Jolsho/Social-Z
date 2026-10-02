/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_client/client.h"

int marshal_get_user_data_request(struct Client* cli, ContextID id);
int marshal_get_post_request(struct Client* cli, ContextID id, int offset);

// Login continuation used after lookup, including a deferred send-buffer return.
int send_user_data_request(struct Client* cli, ContextID id);
