/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "codec/package.h"
#include <assert.h>
#include <string.h>

// A size table precedes three payloads: "abc", an empty blob, and four binary bytes.
static const uint8_t encoded[] = {
    0, 1, 0, 0, 0, 3,
    0, 0, 0, 0, 0, 0, 0, 3,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 4,
    'a', 'b', 'c', 0, 0xff, 1, 2
};

static void chunk_boundaries(void) {
    for (size_t split = 0; split <= sizeof(encoded); split++) {
        PackageParser parser = {.max_size = sizeof(encoded)};
        Package package = {0};
        assert(parse_package_contents(&parser, &package, encoded, split) ==
               (split == sizeof(encoded) ? PACKAGE_DONE : PACKAGE_MORE));
        assert(parse_package_contents(
            &parser, &package, encoded + split, sizeof(encoded) - split
        ) == PACKAGE_DONE);
        assert(finish_package(&parser) == PACKAGE_DONE);
        assert(package.blob_count == 3 && package.size == 7);
        assert(memcmp(package.bytes, encoded + 30, 7) == 0);
        assert(package.blobs[1].size == 0);
        package_destroy(&package);
    }
}

static void malformed(void) {
    for (size_t n = 0; n < sizeof(encoded); n++) {
        PackageParser parser = {.max_size = sizeof(encoded)};
        Package package = {0};
        assert(parse_package_contents(&parser, &package, encoded, n) == PACKAGE_MORE);
        assert(finish_package(&parser) == PACKAGE_ERR);
        assert(parse_package_contents(&parser, &package, encoded, sizeof(encoded)) == PACKAGE_ERR);
        package_destroy(&package);
    }

    const size_t fields[] = {1, 5, 6, 13};
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++) {
        uint8_t invalid[sizeof(encoded)];
        memcpy(invalid, encoded, sizeof(encoded));
        invalid[fields[i]] = 0xff;
        PackageParser parser = {.max_size = sizeof(encoded)};
        Package package = {0};
        assert(parse_package_contents(&parser, &package, invalid, sizeof(invalid)) == PACKAGE_ERR);
        package_destroy(&package);
    }

    PackageParser parser = {.max_size = sizeof(encoded)};
    Package package = {0};
    uint8_t trailing[sizeof(encoded) + 1];
    memcpy(trailing, encoded, sizeof(encoded));
    trailing[sizeof(encoded)] = 0;
    assert(parse_package_contents(&parser, &package, trailing, sizeof(trailing)) == PACKAGE_ERR);
    package_destroy(&package);
}

static void limits_and_empty_blob(void) {
    Package package = {0};
    PackageParser parser = {.max_size = sizeof(encoded) - 1};
    assert(parse_package_contents(&parser, &package, encoded, 30) == PACKAGE_ERR);
    assert(package.bytes == NULL);
    package_destroy(&package);

    parser = (PackageParser){.max_size = 5};
    assert(parse_package_contents(&parser, &package, encoded, 6) == PACKAGE_ERR);
    assert(package.blobs == NULL);

    const uint8_t empty[] = {0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
    parser = (PackageParser){.max_size = sizeof(empty)};
    assert(parse_package_contents(&parser, &package, empty, sizeof(empty)) == PACKAGE_DONE);
    assert(package.blob_count == 1 && package.size == 0 && package.bytes == NULL);
    assert(finish_package(&parser) == PACKAGE_DONE);
    assert(parse_package_contents(&parser, &package, NULL, 0) == PACKAGE_DONE);
    assert(parse_package_contents(&parser, &package, NULL, 1) == PACKAGE_ERR);
    package_destroy(&package);
}

int main(void) {
    chunk_boundaries();
    malformed();
    limits_and_empty_blob();
    return 0;
}
