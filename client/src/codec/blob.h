/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "sz_client/client.h"

// Assemble transfer chunks and verify the whole ciphertext hash before completing.
int parse_blob(struct Client* cli, ContextID id, uint8_t* bytes, uint64_t size);
