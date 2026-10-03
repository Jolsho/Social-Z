/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "codec/blob.h"
#include "codec/encrypted_blob_reader.h"

typedef struct EncryptedBlobLoad {
    BlobTransfer transfer;
    Key key;
    bool started; // The first reply initializes the reader from the complete ciphertext size.
    EncryptedBlobReader reader;
} EncryptedBlobLoad;

// The operation selects this consumer. It must copy any plaintext retained after the call.
typedef int (*PlaintextParser)(struct Client*, ContextID, const uint8_t*, size_t);

// Shared response processing for packages and feed pages; no destination is committed here.
// CLIENT_OK means more ciphertext is pending; CLIENT_PARSE_DONE requires hash and FINAL validation.
int encrypted_blob_read(struct Client* cli, ContextID id, EncryptedBlobLoad* load,
    uint8_t* bytes, uint64_t size, PlaintextParser consume);

// Discard partial ciphertext, wipe stream/key state and scratch; completed cache entries remain.
void encrypted_blob_clear(struct Client* cli, ContextID id, EncryptedBlobLoad* load);
