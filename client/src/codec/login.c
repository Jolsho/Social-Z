/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "codec/login.h"

static uint64_t read_uint(const uint8_t* p, size_t n) {
    uint64_t v = 0;

    for (size_t i = 0; i < n; i++) {
        v = (v << 8) | p[i];
    }

    return v;
}

static void write_uint(
    uint8_t* p,
    uint64_t v,
    size_t n
) {
    while (n) {
        p[--n] = (uint8_t)v;
        v >>= 8;
    }
}

size_t login_username_size(const char* username) {
    if (!username) {
        return 0;
    }

    size_t n = 0;
    while (n <= LOGIN_USERNAME_MAX && username[n]) {
        n++;
    }

    return n && n <= LOGIN_USERNAME_MAX ? n : 0;
}

size_t marshal_account_request(
    uint8_t out[6 + LOGIN_USERNAME_MAX],
    const char* username
) {
    size_t n = login_username_size(username);
    if (!out || !n) {
        return 0;
    }

    write_uint(out, 1, 2);
    write_uint(out + 2, 1, 2);
    write_uint(out + 4, n, 2);
    memcpy(out + 6, username, n);

    return 6 + n;
}

int parse_account_response_metadata(
    AccountResponseMetadata* out,
    const uint8_t* bytes,
    size_t size
) {
    if (!out || !bytes || size != ACCOUNT_RESPONSE_METADATA_SIZE ||
        read_uint(bytes, 2) != 1 || read_uint(bytes + 2, 2) != 1 ||
        read_uint(bytes + 68, 4) != LOGIN_BLOB_SIZE) {
        return -1;
    }

    memcpy(out->account.b, bytes + 4, KEY_SIZE);
    memcpy(out->hash.b, bytes + 36, HASH_SIZE);

    out->size = LOGIN_BLOB_SIZE;

    return 0;
}
