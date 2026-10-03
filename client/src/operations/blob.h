/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "sz_client/client.h"
#include "sz_common/codec.h"

// Starts a GET using the owner's storage label. No decryption happens here.
// Completed ciphertext stays in blob_store under the hash carried by the reply.
int blob_get(struct Client* cli, const Key* owner, const HashT* label, ContextID* id);
// Send GET after the caller installs its operation and parser on an allocated context.
// Failure closes that context through its registered cleanup handler.
int blob_send_get(struct Client* cli, ContextID id, const Key* owner, const HashT* label);
int blob_handle_response(struct Client* cli, ContextID id, uint8_t* bytes, uint64_t size);
void blob_cleanup(struct Client* cli, ContextID id);
