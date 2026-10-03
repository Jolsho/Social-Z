/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "codec/encrypted_blob.h"

#define ENCRYPTED_BLOB_ERR -1
#define ENCRYPTED_BLOB_MORE 0
#define ENCRYPTED_BLOB_CHUNK 1
#define ENCRYPTED_BLOB_DONE 2

typedef struct EncryptedBlobReader {
    BlobCrypt crypt;
    uint64_t remaining; // Initialize to the complete stored ciphertext size.
    // Retain a split envelope header or length field without borrowing the host's bytes.
    uint8_t field[ENCRYPTED_BLOB_HEADER_SIZE];
    size_t field_received;
    // Chunk counters include the length prefix; ciphertext bytes live in caller-owned scratch.
    uint32_t chunk_size;
    uint32_t chunk_received;
    enum {
        ENCRYPTED_READ_HEADER,
        ENCRYPTED_READ_LENGTH,
        ENCRYPTED_READ_CHUNK,
        ENCRYPTED_READ_END,
        ENCRYPTED_READ_FAILED,
    } state;
} EncryptedBlobReader;

/* Start with a zeroed reader whose remaining is the declared encrypted blob size.
 * Supply owned scratch storage; keep its bytes, allocation, and capacity across MORE calls.
 * Input is borrowed and must not overlap scratch storage or reader state.
 * No allocation. Capacity must fit each complete encrypted chunk, including its prefix.
 * consumed tells the caller where to resume if input contains several encrypted chunks.
 * CHUNK/DONE expose one authenticated plaintext slice in buffer; MORE exposes size=0.
 * Feed those slices to staging content before the next call reuses the same buffer.
 * DONE includes the final slice; commit only after whole-ciphertext hash verification too.
 * ERR is terminal. Cancellation must clear reader.crypt.crypt and discard staging content.
 */
int parse_encrypted_blob(
    EncryptedBlobReader* reader,
    const Key* key,
    Buffer* buffer,
    const uint8_t* bytes,
    size_t size,
    size_t* consumed
);

// Check actual end of input; missing bytes or a missing FINAL tag fail and close the stream.
int finish_encrypted_blob(EncryptedBlobReader* reader);
