/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "codec/account.h"
#include <assert.h>

static void exact_format(void) {
    AccountHeader header = {
        .first_feed_page = UINT64_C(0x0102030405060708),
        .current_feed_page = UINT64_C(0x1112131415161718),
        .first_recipient_page = UINT64_C(0x2122232425262728),
        .current_recipient_page = UINT64_C(0x3132333435363738),
    };

    uint8_t expected[ACCOUNT_HEADER_SIZE] = {
        0, 1, 0, 2,
        1, 2, 3, 4, 5, 6, 7, 8,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
    };

    for (size_t i = 0; i < HASH_SIZE; i++) {
        header.inbox_head_locator.b[i] = (uint8_t)(0xa0 + i);
        expected[36 + i] = (uint8_t)(0xa0 + i);
        header.signing_seed.b[i] = expected[68 + i] = (uint8_t)(0x20 + i);
        header.data_key.b[i] = expected[100 + i] = (uint8_t)(0x60 + i);
    }

    uint8_t bytes[ACCOUNT_HEADER_SIZE];
    size_t size = 0;

    assert(marshal_account_header(bytes, sizeof(bytes), &size, &header) == 0);
    assert(size == sizeof(expected));
    assert(memcmp(bytes, expected, size) == 0);

    AccountHeader parsed;
    assert(parse_account_header(&parsed, bytes, size) == 0);

    // Parsed fields must remain valid after the host reuses its input memory.
    memset(bytes, 0, sizeof(bytes));

    assert(parsed.first_feed_page == header.first_feed_page);
    assert(parsed.current_feed_page == header.current_feed_page);
    assert(parsed.first_recipient_page == header.first_recipient_page);
    assert(parsed.current_recipient_page == header.current_recipient_page);
    assert(memcmp(parsed.inbox_head_locator.b, header.inbox_head_locator.b, HASH_SIZE) == 0);
    assert(memcmp(parsed.signing_seed.b, header.signing_seed.b, KEY_SIZE) == 0);
    assert(memcmp(parsed.data_key.b, header.data_key.b, KEY_SIZE) == 0);

    assert(marshal_account_header(bytes, sizeof(bytes), &size, &parsed) == 0);
    assert(memcmp(bytes, expected, size) == 0);
}

static void reject_parse(const uint8_t* bytes, size_t size) {
    AccountHeader out;
    uint8_t before[sizeof(out)];

    memset(&out, 0xa5, sizeof(out));
    memcpy(before, &out, sizeof(out));

    assert(parse_account_header(&out, bytes, size) == -1);
    assert(memcmp(&out, before, sizeof(out)) == 0);
}

static void malformed_records(void) {
    AccountHeader header = {0};
    uint8_t bytes[ACCOUNT_HEADER_SIZE + 1] = {0};
    size_t size;

    assert(marshal_account_header(bytes, sizeof(bytes), &size, &header) == 0);

    for (size_t n = 0; n < ACCOUNT_HEADER_SIZE; n++) {
        reject_parse(bytes, n);
    }

    reject_parse(bytes, sizeof(bytes));
    reject_parse(bytes, SIZE_MAX);
    reject_parse(NULL, ACCOUNT_HEADER_SIZE);
    assert(parse_account_header(NULL, bytes, size) == -1);

    for (size_t i = 0; i < 4; i++) {
        bytes[i] ^= 1;
        reject_parse(bytes, size);
        bytes[i] ^= 1;
    }

    bytes[11] = 1;
    reject_parse(bytes, size);
    bytes[11] = 0;

    bytes[27] = 1;
    reject_parse(bytes, size);
}

static void reject_marshal(const AccountHeader* header, size_t capacity) {
    uint8_t bytes[ACCOUNT_HEADER_SIZE];
    size_t size = 999;

    memset(bytes, 0xa5, sizeof(bytes));

    assert(marshal_account_header(bytes, capacity, &size, header) == -1);
    assert(size == 999);

    for (size_t i = 0; i < sizeof(bytes); i++) {
        assert(bytes[i] == 0xa5);
    }
}

static void page_bounds(void) {
    AccountHeader header = {
        .current_feed_page = UINT64_MAX,
        .current_recipient_page = UINT64_MAX,
    };

    uint8_t bytes[ACCOUNT_HEADER_SIZE];
    AccountHeader parsed;
    size_t size;

    assert(marshal_account_header(bytes, sizeof(bytes), &size, &header) == 0);
    assert(parse_account_header(&parsed, bytes, size) == 0);
    assert(parsed.first_feed_page == 0 && parsed.current_feed_page == UINT64_MAX);
    assert(parsed.first_recipient_page == 0 && parsed.current_recipient_page == UINT64_MAX);

    header.first_feed_page = UINT64_MAX;
    header.first_recipient_page = UINT64_MAX;

    assert(marshal_account_header(bytes, sizeof(bytes), &size, &header) == 0);
    assert(parse_account_header(&parsed, bytes, size) == 0);
    assert(parsed.first_feed_page == UINT64_MAX && parsed.first_recipient_page == UINT64_MAX);

    reject_marshal(&header, ACCOUNT_HEADER_SIZE - 1);
    reject_marshal(NULL, ACCOUNT_HEADER_SIZE);
    assert(marshal_account_header(NULL, sizeof(bytes), &size, &header) == -1);
    assert(marshal_account_header(bytes, sizeof(bytes), NULL, &header) == -1);

    header.current_feed_page = UINT64_MAX - 1;
    reject_marshal(&header, sizeof(bytes));

    header.current_feed_page = UINT64_MAX;
    header.current_recipient_page = UINT64_MAX - 1;
    reject_marshal(&header, sizeof(bytes));
}

int main(void) {
    exact_format();
    malformed_records();
    page_bounds();

    return 0;
}
