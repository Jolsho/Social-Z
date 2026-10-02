/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "codec/login.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ACCOUNT_HEADER_SIZE 132

typedef struct AccountHeader {
    uint64_t first_feed_page;
    uint64_t current_feed_page;

    uint64_t first_recipient_page;
    uint64_t current_recipient_page;

    HashT inbox_head_locator;

    Key signing_seed;
    Key data_key;
} AccountHeader;

/* V1, kind 2 plaintext. Integers are big-endian; first pages cannot exceed current pages.
 * Encryption uses the password-derived key; data_key encrypts private data blobs.
 * Return 0 or -1. Failed calls leave outputs unchanged.
 */
int marshal_account_header(
    uint8_t* out,
    size_t capacity,
    size_t* size,
    const AccountHeader* header
);

int parse_account_header(
    AccountHeader* out,
    const uint8_t* bytes,
    size_t size
);

/* Passwords stay local. Fresh salt and stream header on every encryption.
 * Decryption binds the recovered signing identity to the username lookup's account key.
 * Borrowed inputs are not retained. Return 0 or -1; failed calls leave outputs unchanged.
 */
int encrypt_account_header(
    uint8_t out[LOGIN_BLOB_SIZE],
    const char* username,
    const uint8_t* password,
    size_t password_size,
    const AccountHeader* header
);

int decrypt_account_header(
    AccountHeader* header,
    KeyPair* keys,
    const Key* account,
    const char* username,
    const uint8_t* password,
    size_t password_size,
    const uint8_t* blob,
    size_t blob_size
);

#ifdef __cplusplus
}
#endif
