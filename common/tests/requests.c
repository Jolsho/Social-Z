/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/requests/requests.h"
#include <assert.h>

static void account_requests(void) {
    uint8_t bytes[7 + ACCOUNT_USERNAME_MAX];
    const uint8_t expected[] = {0, 1, 0, 1, 0, 5, 'a', 'l', 'i', 'c', 'e'};
    Request request = {0};
    char* username = request.data.username;

    assert(marshal_account_request(bytes, "alice") == sizeof(expected));
    assert(memcmp(bytes, expected, sizeof(expected)) == 0);
    assert(parse_request(&request, bytes, sizeof(expected)) == 0);
    assert(request.kind == REQUEST_ACCOUNT);
    assert(strcmp(username, "alice") == 0);

    for (size_t size = 0; size < sizeof(expected); size++) {
        strcpy(username, "unchanged");
        assert(parse_request(&request, bytes, size) == -1);
        assert(strcmp(username, "unchanged") == 0);
    }
    assert(parse_request(&request, bytes, sizeof(expected) + 1) == -1);

    const size_t invalid_fields[] = {1, 3, 5, 6};
    for (size_t i = 0; i < sizeof(invalid_fields) / sizeof(invalid_fields[0]); i++) {
        memcpy(bytes, expected, sizeof(expected));
        bytes[invalid_fields[i]] = 0;
        assert(parse_request(&request, bytes, sizeof(expected)) == -1);
        assert(strcmp(username, "unchanged") == 0);
    }

    memset(username, 'a', ACCOUNT_USERNAME_MAX);
    username[ACCOUNT_USERNAME_MAX] = 0;
    size_t size = marshal_account_request(bytes, username);
    assert(size == 6 + ACCOUNT_USERNAME_MAX);
    memset(username, 0, ACCOUNT_USERNAME_MAX + 1);
    assert(parse_request(&request, bytes, size) == 0);
    assert(strlen(username) == ACCOUNT_USERNAME_MAX);

    bytes[5] = ACCOUNT_USERNAME_MAX + 1;
    assert(parse_request(&request, bytes, size + 1) == -1);
    char too_long[ACCOUNT_USERNAME_MAX + 2];
    memset(too_long, 'a', sizeof(too_long) - 1);
    too_long[sizeof(too_long) - 1] = 0;
    bytes[0] = 0xaa;
    assert(marshal_account_request(bytes, too_long) == 0);
    assert(marshal_account_request(bytes, "") == 0);
    assert(bytes[0] == 0xaa);
    assert(parse_request(&request, NULL, 6) == -1);
}

static void account_responses(void) {
    AccountResponseMetadata metadata = {.size = 0x01020304};
    memset(metadata.account.b, 0xaa, KEY_SIZE);
    memset(metadata.hash.b, 0xbb, HASH_SIZE);
    uint8_t bytes[ACCOUNT_RESPONSE_METADATA_SIZE + 1];
    marshal_account_response_metadata(bytes, &metadata);
    assert(bytes[0] == 0 && bytes[1] == 1 && bytes[2] == 0 && bytes[3] == 1);
    assert(memcmp(bytes + 4, metadata.account.b, KEY_SIZE) == 0);
    assert(memcmp(bytes + 36, metadata.hash.b, HASH_SIZE) == 0);
    const uint8_t size_bytes[] = {1, 2, 3, 4};
    assert(memcmp(bytes + 68, size_bytes, sizeof(size_bytes)) == 0);

    AccountResponseMetadata parsed = {0};
    assert(parse_account_response_metadata(&parsed, bytes, ACCOUNT_RESPONSE_METADATA_SIZE) == 0);
    assert(memcmp(&parsed, &metadata, sizeof(metadata)) == 0);
    for (size_t size = 0; size < ACCOUNT_RESPONSE_METADATA_SIZE; size++) {
        assert(parse_account_response_metadata(&parsed, bytes, size) == -1);
        assert(memcmp(&parsed, &metadata, sizeof(metadata)) == 0);
    }
    assert(parse_account_response_metadata(&parsed, bytes, sizeof(bytes)) == -1);
    bytes[3] = 2;
    assert(parse_account_response_metadata(&parsed, bytes, ACCOUNT_RESPONSE_METADATA_SIZE) == -1);
    bytes[3] = 1;
    bytes[1] = 2;
    assert(parse_account_response_metadata(&parsed, bytes, ACCOUNT_RESPONSE_METADATA_SIZE) == -1);
    bytes[1] = 1;
    memset(bytes + 68, 0, 4);
    assert(parse_account_response_metadata(&parsed, bytes, ACCOUNT_RESPONSE_METADATA_SIZE) == -1);
    assert(memcmp(&parsed, &metadata, sizeof(metadata)) == 0);
}

static void request_prefixes(void) {
    uint8_t bytes[] = {0, 1, 0, REQUEST_ACCOUNT, 0, 1, 'a'};
    Request request;
    memset(&request, 0xaa, sizeof(request));
    uint8_t before[sizeof(request)];
    memcpy(before, &request, sizeof(request));

    bytes[2] = bytes[3] = 0xff;
    assert(parse_request(&request, bytes, sizeof(bytes)) == -1);
    assert(memcmp(&request, before, sizeof(request)) == 0);

    bytes[2] = 0;
    bytes[3] = REQUEST_ACCOUNT;
    bytes[1] = 2;
    assert(parse_request(&request, bytes, sizeof(bytes)) == -1);
    assert(memcmp(&request, before, sizeof(request)) == 0);
    assert(parse_request(NULL, bytes, sizeof(bytes)) == -1);
}

int main(void) {
    account_requests();
    account_responses();
    request_prefixes();
    return 0;
}
