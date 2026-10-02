/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_common/codec.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOGIN_USERNAME_MAX 64
#define LOGIN_PASSWORD_MAX 1024
#define LOGIN_BLOB_SIZE 213
#define ACCOUNT_RESPONSE_METADATA_SIZE 72

/* V1 bootstrap messages use big-endian fields: version:u16, operation:u16.
 * Account request (1): username length:u16, bytes.
 * Reply (1): account key, ciphertext hash, size:u32, followed by blob transfer chunks.
 * The node resolves the username and streams the encrypted header without another request.
 */
typedef struct AccountResponseMetadata {
    Key account;
    HashT hash;
    uint32_t size;
} AccountResponseMetadata;

size_t login_username_size(const char* username);
size_t marshal_account_request(
    uint8_t out[6 + LOGIN_USERNAME_MAX],
    const char* username
);
int parse_account_response_metadata(
    AccountResponseMetadata* out,
    const uint8_t* bytes,
    size_t size
);

#ifdef __cplusplus
}
#endif
