/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_common/codec.h"
#include "sz_common/requests/blob.h"
#include "sz_common/requests/hash_batch.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ACCOUNT_USERNAME_MAX 64
#define ACCOUNT_RESPONSE_METADATA_SIZE 72
#define REQUEST_HEADER_SIZE 4
#define HASH_BATCH_HEADER_SIZE (REQUEST_HEADER_SIZE + KEY_SIZE + sizeof(uint32_t))
#define HASH_BATCH_REQUEST_BASE_SIZE (HASH_BATCH_HEADER_SIZE + SIGNATURE_SIZE)
#define BLOB_GET_REQUEST_SIZE (REQUEST_HEADER_SIZE + KEY_SIZE + HASH_SIZE)
#define BLOB_DELETE_REQUEST_SIZE (BLOB_GET_REQUEST_SIZE + SIGNATURE_SIZE)
#define BLOB_PUT_REQUEST_SIZE (BLOB_DELETE_REQUEST_SIZE + HASH_SIZE)

typedef enum RequestKind {
    REQUEST_ACCOUNT = 1,
    REQUEST_BLOB_GET = 2,
    REQUEST_BLOB_PUT = 3,
    REQUEST_BLOB_DELETE = 4,
    REQUEST_NEW_PERMISSIONS = 5,
    REQUEST_PUBLISH_VOUCHERS = 6,
    REQUEST_REVOKE_PERMISSIONS = 7,
} RequestKind;

typedef struct Request {
    RequestKind kind;
    union {
        char username[ACCOUNT_USERNAME_MAX + 1];
        BlobRequest blob;
        HashBatch hash_batch;
    } data;
} Request;

/* Every request starts with version:u16=1 and kind:u16, both big-endian.
 * ACCOUNT: username length:u16, then username bytes.
 * Account replies carry metadata followed by encrypted blob transfer chunks.
 * BLOB_GET: owner:32, label:32.
 * BLOB_DELETE: owner:32, label:32, signature:64.
 * BLOB_PUT: owner:32, label:32, ciphertext_hash:32, signature:64.
 * PUT declares insertion; ciphertext transfer and commit are separate work.
 * NEW_PERMISSIONS / REVOKE_PERMISSIONS: owner:32, count:u32, hashes:32 each, signature:64.
 * PUBLISH_VOUCHERS: owner:32, count:u32, expires:u64, hashes:32 each, signature:64.
 * Voucher expiry is one UTC timestamp in seconds for the complete batch, encoded big-endian.
 */
typedef struct AccountResponseMetadata {
    Key account;
    HashT hash;
    uint32_t size;
} AccountResponseMetadata;

size_t account_username_size(const char* username);

// Marshal into caller-owned storage of the declared size; no allocation or sending.
size_t marshal_account_request(
    uint8_t out[6 + ACCOUNT_USERNAME_MAX],
    const char* username
);
// Supply storage for the selected kind (at most BLOB_PUT_REQUEST_SIZE).
// Returns bytes written, or zero for an unsupported kind. Does not sign or allocate.
// Arguments must be valid and output must not overlap the request.
size_t marshal_blob_request(
    uint8_t* out,
    const Request* request
);
// Supply BASE_SIZE + count * HASH_SIZE bytes, plus eight expiry bytes for vouchers.
// Hashes and output must not overlap; no signing, allocation, or sending occurs here.
// Returns bytes written, or zero for unsupported kind or unrepresentable size.
size_t marshal_hash_batch_request(uint8_t* out, const Request* request);

// Dispatch a complete request by its prefix and copy its fields into caller storage.
// Usernames are NUL-terminated. Failures leave output unchanged.
// Input must not overlap output or its hash storage; no input pointers are retained.
// Parsing checks structure only; verify mutation signatures separately.
// For batches, initialize data.hash_batch.hashes and capacity before calling.
// Hashes are copied into that storage; insufficient capacity leaves output unchanged.
int parse_request(Request* out, const uint8_t* bytes, size_t size);

// The node supplies an opaque blob's identity and size, then sends its chunks.
void marshal_account_response_metadata(
    uint8_t out[ACCOUNT_RESPONSE_METADATA_SIZE],
    const AccountResponseMetadata* metadata
);
int parse_account_response_metadata(
    AccountResponseMetadata* out,
    const uint8_t* bytes,
    size_t size
);

#ifdef __cplusplus
}
#endif
