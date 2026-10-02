/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "codec/package.h"
#include <assert.h>
#include <string.h>

// Three blobs: post data "abc", an empty attachment, and four binary bytes.
static const uint8_t package[] = {
    0, 1, 0, 0, 0, 3,
    0, 0, 0, 0, 0, 0, 0, 3, 'a', 'b', 'c',
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 4, 0, 0xff, 1, 2
};

static void consume(PackageParser* parser, const uint8_t* bytes, size_t size,
                    uint8_t output[3][4], size_t lengths[3], size_t* empty_count) {
    size_t offset = 0;
    for (;;) {
        PackageSlice blob;
        size_t consumed;
        int status = parse_package(parser, bytes + offset, size - offset, &consumed, &blob);
        assert(consumed <= size - offset);
        offset += consumed;
        if (status != PACKAGE_BLOB) {
            assert(status == PACKAGE_MORE || status == PACKAGE_DONE);
            assert(offset == size);
            return;
        }

        static const size_t expected_sizes[] = {3, 0, 4};
        assert(blob.index < 3);
        assert(blob.total_size == expected_sizes[blob.index]);
        assert(blob.offset == lengths[blob.index]);
        assert(blob.size <= expected_sizes[blob.index] - lengths[blob.index]);
        if (blob.size) {
            assert(blob.bytes >= bytes && blob.bytes + blob.size <= bytes + size);
            memcpy(output[blob.index] + blob.offset, blob.bytes, blob.size);
        } else {
            assert(blob.index == 1 && blob.bytes == NULL);
            (*empty_count)++;
        }
        lengths[blob.index] += blob.size;
    }
}

static void chunk_boundaries(void) {
    for (size_t split = 0; split <= sizeof(package); split++) {
        PackageParser parser = {0};
        uint8_t output[3][4] = {{0}};
        size_t lengths[3] = {0}, empty_count = 0;
        consume(&parser, package, split, output, lengths, &empty_count);
        consume(&parser, package + split, sizeof(package) - split, output, lengths, &empty_count);
        assert(finish_package(&parser) == PACKAGE_DONE);
        assert(parser.blob_count == 3 && parser.blob_index == 3);
        assert(memcmp(output[0], "abc", 3) == 0);
        const uint8_t binary[] = {0, 0xff, 1, 2};
        assert(memcmp(output[2], binary, sizeof(binary)) == 0);
        assert(empty_count == 1);
    }

    PackageParser parser = {0};
    uint8_t output[3][4] = {{0}};
    size_t lengths[3] = {0}, empty_count = 0;
    for (size_t i = 0; i < sizeof(package); i++) {
        // The same incoming buffer is reused; the consumer copies retained slices.
        uint8_t borrowed = package[i];
        consume(&parser, &borrowed, 1, output, lengths, &empty_count);
        borrowed = 0;
    }
    assert(finish_package(&parser) == PACKAGE_DONE);
    assert(memcmp(output[0], "abc", 3) == 0 && empty_count == 1);
}

static int drain(PackageParser* parser, const uint8_t* bytes, size_t size) {
    size_t offset = 0;
    int status;
    do {
        size_t consumed;
        PackageSlice blob;
        status = parse_package(parser, bytes + offset, size - offset, &consumed, &blob);
        offset += consumed;
    } while (status == PACKAGE_BLOB);
    return status;
}

static void malformed(void) {
    for (size_t length = 0; length < sizeof(package); length++) {
        PackageParser parser = {0};
        assert(drain(&parser, package, length) == PACKAGE_MORE);
        assert(finish_package(&parser) == PACKAGE_ERR);
        assert(drain(&parser, package, sizeof(package)) == PACKAGE_ERR);
    }

    uint8_t bytes[sizeof(package) + 1];
    memcpy(bytes, package, sizeof(package));
    PackageParser parser = {0};
    bytes[1] = 2;
    assert(drain(&parser, bytes, sizeof(package)) == PACKAGE_ERR);
    parser = (PackageParser){0};
    bytes[1] = 1;
    memset(bytes + 2, 0, 4);
    assert(drain(&parser, bytes, sizeof(package)) == PACKAGE_ERR);

    memcpy(bytes, package, sizeof(package));
    bytes[sizeof(package)] = 0;
    parser = (PackageParser){0};
    assert(drain(&parser, bytes, sizeof(bytes)) == PACKAGE_ERR);
    assert(finish_package(&parser) == PACKAGE_ERR);

    memcpy(bytes, package, sizeof(package));
    bytes[5] = 2; // Declared count ends before the third blob.
    parser = (PackageParser){0};
    assert(drain(&parser, bytes, sizeof(package)) == PACKAGE_ERR);
    bytes[5] = 4;
    parser = (PackageParser){0};
    assert(drain(&parser, bytes, sizeof(package)) == PACKAGE_MORE);
    assert(finish_package(&parser) == PACKAGE_ERR);
}

static void large_lengths_and_empty_blob(void) {
    const uint8_t header[] = {0, 1, 0, 0, 0, 1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    PackageParser parser = {0};
    assert(drain(&parser, header, sizeof(header)) == PACKAGE_MORE);
    const uint8_t byte = 42;
    PackageSlice blob;
    size_t consumed;
    assert(parse_package(&parser, &byte, 1, &consumed, &blob) == PACKAGE_BLOB);
    assert(consumed == 1 && blob.total_size == UINT64_MAX && blob.offset == 0);
    assert(blob.size == 1 && blob.bytes == &byte);
    assert(finish_package(&parser) == PACKAGE_ERR);

    uint8_t empty[14] = {0, 1, 0, 0, 0, 1};
    parser = (PackageParser){0};
    assert(parse_package(&parser, empty, sizeof(empty), &consumed, &blob) == PACKAGE_BLOB);
    assert(consumed == sizeof(empty) && blob.total_size == 0 && blob.size == 0);
    assert(finish_package(&parser) == PACKAGE_DONE);
    assert(parse_package(&parser, NULL, 0, &consumed, &blob) == PACKAGE_DONE);
    assert(parse_package(&parser, NULL, 1, &consumed, &blob) == PACKAGE_ERR);
    assert(finish_package(NULL) == PACKAGE_ERR);
}

int main(void) {
    chunk_boundaries();
    malformed();
    large_lengths_and_empty_blob();
    return 0;
}
