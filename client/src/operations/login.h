/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "sz_client/client.h"

// Response dispatch advances the username request through metadata and header chunks.
int login_handle_response(struct Client* cli, ContextID id, uint8_t* bytes, uint64_t size);
