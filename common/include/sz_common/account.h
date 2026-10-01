/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "sz_common/codec.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ACCOUNT_HEADER_SIZE 68

typedef struct AccountHeader {
    uint64_t first_feed_page;
    uint64_t current_feed_page;

    uint64_t first_recipient_page;
    uint64_t current_recipient_page;

    HashT inbox_head_locator;
} AccountHeader;

/* V1, kind 2 plaintext. Integers are big-endian; first pages cannot exceed current pages.
 * Encryption under user_data_key is handled separately.
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

#ifdef __cplusplus
}
#endif
