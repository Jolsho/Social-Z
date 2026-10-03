/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "sz_client/client.h"
#include "sz_common/codec.h"

typedef struct BlobTransfer {
    bool active;
    HashT hash;
} BlobTransfer;

// Assemble transfer chunks and verify the whole ciphertext hash before completing.
// Transfer is initialized and owned by the operation; no context fields are used for progress.
int parse_blob(struct Client* cli, ContextID id, BlobTransfer* transfer,
    uint8_t* bytes, uint64_t size);

// Discard only this context's partial transfer; completed cached blobs remain available.
void blob_discard_partial(struct Client* cli, ContextID id, BlobTransfer* transfer);
