/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/account.h"
#include <stdbool.h>

static uint64_t read_u64(const uint8_t* bytes) {
    uint64_t value = 0;

    for (size_t i = 0; i < 8; i++) {
        value = (value << 8) | bytes[i];
    }

    return value;
}

static void write_u64(uint8_t* bytes, uint64_t value) {
    for (size_t i = 8; i > 0; i--) {
        bytes[i - 1] = (uint8_t)value;
        value >>= 8;
    }
}

static bool valid_pages(const AccountHeader* header) {
    return header->first_feed_page <= header->current_feed_page &&
           header->first_recipient_page <= header->current_recipient_page;
}

int marshal_account_header(
    uint8_t* out,
    size_t capacity,
    size_t* size,
    const AccountHeader* header
) {
    if (!out || !size || !header || capacity < ACCOUNT_HEADER_SIZE) {
        return -1;
    }

    if (!valid_pages(header)) {
        return -1;
    }

    // Big-endian u16 fields: version 1, then record kind 2 (account header).
    out[0] = 0;
    out[1] = 1;
    out[2] = 0;
    out[3] = 2;

    write_u64(out + 4, header->first_feed_page);
    write_u64(out + 12, header->current_feed_page);

    write_u64(out + 20, header->first_recipient_page);
    write_u64(out + 28, header->current_recipient_page);

    memcpy(out + 36, header->inbox_head_locator.b, HASH_SIZE);

    *size = ACCOUNT_HEADER_SIZE;
    return 0;
}

int parse_account_header(
    AccountHeader* out,
    const uint8_t* bytes,
    size_t size
) {
    if (!out || !bytes || size != ACCOUNT_HEADER_SIZE) {
        return -1;
    }

    if (bytes[0] != 0 || bytes[1] != 1 || bytes[2] != 0 || bytes[3] != 2) {
        return -1;
    }

    AccountHeader header = {0};

    header.first_feed_page = read_u64(bytes + 4);
    header.current_feed_page = read_u64(bytes + 12);

    header.first_recipient_page = read_u64(bytes + 20);
    header.current_recipient_page = read_u64(bytes + 28);

    memcpy(header.inbox_head_locator.b, bytes + 36, HASH_SIZE);

    if (!valid_pages(&header)) {
        return -1;
    }

    *out = header;
    return 0;
}
