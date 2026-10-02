/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_common/codec.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ACCOUNT_USERNAME_MAX 64
#define ACCOUNT_RESPONSE_METADATA_SIZE 72
#define REQUEST_HEADER_SIZE 4

typedef enum RequestKind {
    REQUEST_ACCOUNT = 1,
} RequestKind;

typedef struct Request {
    RequestKind kind;
    char username[ACCOUNT_USERNAME_MAX + 1];
} Request;

/* Every request starts with version:u16=1 and kind:u16, both big-endian.
 * ACCOUNT: username length:u16, then username bytes.
 * Account replies carry metadata followed by encrypted blob transfer chunks.
 * Content operations will address opaque blobs, not posts or feed offsets.
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
// Dispatch a complete request by its prefix and copy its fields into caller storage.
// Usernames are NUL-terminated. Failures leave output unchanged.
// Input and output must not overlap; no input pointers are retained.
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
