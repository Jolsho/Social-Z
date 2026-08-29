/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "client.h"

int marshal_get_user_data_request(Client* cli, ContextID id, Key* pub_key);

int marshal_get_post_request(Client* cli, ContextID id, int offset);
