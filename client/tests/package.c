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
    PackageMarshaler marshaler = {0};
    uint8_t output[sizeof(empty)];
    size_t written;
    assert(marshal_package(&marshaler, &package, output, sizeof(output), &written) == PACKAGE_DONE);
    assert(written == sizeof(empty) && memcmp(output, empty, sizeof(empty)) == 0);
    assert(parse_package_contents(&parser, &package, NULL, 0) == PACKAGE_DONE);
    assert(parse_package_contents(&parser, &package, NULL, 1) == PACKAGE_ERR);
    package_destroy(&package);
}

static void marshal_chunks(void) {
    Package source = {0};
    PackageParser source_parser = {.max_size = sizeof(encoded)};
    assert(parse_package_contents(&source_parser, &source, encoded, sizeof(encoded)) == PACKAGE_DONE);

    for (size_t capacity = 1; capacity <= sizeof(encoded) + 3; capacity++) {
        PackageMarshaler marshaler = {0};
        PackageParser parser = {.max_size = sizeof(encoded)};
        Package roundtrip = {0};
        uint8_t chunk[sizeof(encoded) + 3];
        size_t offset = 0;
        int status;

        do {
            memset(chunk, 0xaa, sizeof(chunk));
            size_t written;
            status = marshal_package(&marshaler, &source, chunk, capacity, &written);
            assert(status == PACKAGE_MORE || status == PACKAGE_DONE);
            assert(written <= capacity && written <= sizeof(encoded) - offset);
            assert(memcmp(chunk, encoded + offset, written) == 0);
            for (size_t i = written; i < sizeof(chunk); i++) {
                assert(chunk[i] == 0xaa);
            }
            offset += written;
            assert(parse_package_contents(&parser, &roundtrip, chunk, written) ==
                   (status == PACKAGE_DONE ? PACKAGE_DONE : PACKAGE_MORE));
        } while (status != PACKAGE_DONE);

        assert(offset == sizeof(encoded));
        assert(finish_package(&parser) == PACKAGE_DONE);
        assert(memcmp(roundtrip.bytes, source.bytes, source.size) == 0);
        size_t written = 99;
        assert(marshal_package(&marshaler, &source, NULL, 0, &written) == PACKAGE_DONE);
        assert(written == 0);
        package_destroy(&roundtrip);
    }
    package_destroy(&source);
}

static void invalid_marshal_input(void) {
    Package source = {0};
    PackageParser parser = {.max_size = sizeof(encoded)};
    assert(parse_package_contents(&parser, &source, encoded, sizeof(encoded)) == PACKAGE_DONE);
    PackageMarshaler marshaler = {0};
    uint8_t output[sizeof(encoded)];
    memset(output, 0xaa, sizeof(output));
    size_t written = 99;

    source.blobs[0].size = SIZE_MAX;
    assert(marshal_package(&marshaler, &source, output, sizeof(output), &written) == PACKAGE_ERR);
    assert(written == 99 && marshaler.offset == 0);
    for (size_t i = 0; i < sizeof(output); i++) {
        assert(output[i] == 0xaa);
    }
    source.blobs[0].size = 3;

    uint8_t* bytes = source.blobs[0].bytes;
    source.blobs[0].bytes = NULL;
    assert(marshal_package(&marshaler, &source, output, sizeof(output), &written) == PACKAGE_ERR);
    source.blobs[0].bytes = bytes;
    source.size = SIZE_MAX;
    assert(marshal_package(&marshaler, &source, output, sizeof(output), &written) == PACKAGE_ERR);
    source.size = 7;
    assert(marshal_package(&marshaler, &source, NULL, 1, &written) == PACKAGE_ERR);
    assert(marshal_package(&marshaler, &source, NULL, 0, &written) == PACKAGE_MORE);
    assert(written == 0 && marshaler.offset == 0);
    package_destroy(&source);
}

int main(void) {
    chunk_boundaries();
    malformed();
    limits_and_empty_blob();
    marshal_chunks();
    invalid_marshal_input();
    return 0;
}
