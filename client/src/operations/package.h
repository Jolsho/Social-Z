/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "sz_client/client.h"
#include "sz_common/codec.h"
#include "content/package.h"

// Start an asynchronous blob GET, decrypt its replies, and load their contents into destination.
// CLIENT_OK means the request started; client_parse_response reports completion or failure later.
// Destination must be initialized and remain at the same address until completion/cancellation.
// The operation copies the key. Failure leaves destination unchanged.
// max_size bounds complete package plaintext, including its framing.
int package_get(
    struct Client* cli,
    const Key* owner,
    const HashT* label,
    const Key* key,
    Package* destination,
    size_t max_size,
    ContextID* id
);

int package_handle_response(struct Client* cli, ContextID id, uint8_t* bytes, uint64_t size);
void package_cleanup(struct Client* cli, ContextID id);
