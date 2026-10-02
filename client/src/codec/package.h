/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#define PACKAGE_ERR -1
#define PACKAGE_MORE 0
#define PACKAGE_BLOB 1
#define PACKAGE_DONE 2

typedef struct PackageParser {
    uint8_t field[8];
    size_t field_size;
    uint32_t blob_count;
    uint32_t blob_index;
    uint64_t blob_size;
    uint64_t blob_offset;
    enum { PACKAGE_HEADER, PACKAGE_LENGTH, PACKAGE_DATA, PACKAGE_END, PACKAGE_FAILED } state;
} PackageParser;

typedef struct PackageSlice {
    uint32_t index;
    uint64_t total_size;
    uint64_t offset;
    const uint8_t* bytes;
    size_t size;
} PackageSlice;

/* Plaintext V1: version:u16, blob_count:u32, then length:u64 and bytes per blob.
 * Integers are big-endian; count is nonzero. Blob zero is the post-data blob.
 * Start with a zero-initialized parser. No allocation or payload retention occurs.
 * Each call consumes input up to one borrowed blob slice; resume with remaining bytes.
 * Empty blobs yield one zero-size slice. MORE means another input chunk is needed.
 * Call finish_package() at end of input to reject truncation; trailing bytes are errors.
 * Slices must be copied if retained. They are provisional until parsing and authentication finish.
 * ERR is terminal; reset the parser before reading another package.
 */
int parse_package(
    PackageParser* parser,
    const uint8_t* bytes,
    size_t size,
    size_t* consumed,
    PackageSlice* blob
);

int finish_package(PackageParser* parser);
