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
#define LOGIN_BLOB_SIZE 149
#define LOGIN_LOOKUP_SIZE 72
#define LOGIN_FETCH_SIZE 36

/* V1 bootstrap messages use big-endian fields: version:u16, operation:u16.
 * Lookup (1): username length:u16, bytes. Fetch (2): ciphertext hash.
 * Lookup reply (1): account key, ciphertext hash, size:u32.
 */
typedef struct LoginLookup {
    Key account;
    HashT hash;
    uint32_t size;
} LoginLookup;

size_t login_username_size(const char* username);
size_t login_lookup_request(uint8_t out[6 + LOGIN_USERNAME_MAX], const char* username);
void login_fetch_request(uint8_t out[LOGIN_FETCH_SIZE], const HashT* hash);
int login_read_lookup(LoginLookup* out, const uint8_t* bytes, size_t size);

/* Passwords stay local. Outputs are changed only on success.
 * The encrypted record contains a signing seed and the main owner-data key.
 */
int login_encrypt(uint8_t out[LOGIN_BLOB_SIZE], const char* username,
    const uint8_t* password, size_t password_size, const KeyPair* keys, const Key* data_key);
int login_decrypt(KeyPair* keys, Key* data_key, const Key* account, const char* username,
    const uint8_t* password, size_t password_size, const uint8_t* blob, size_t blob_size);

#ifdef __cplusplus
}
#endif
