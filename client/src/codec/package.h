/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "content/package.h"

#define PACKAGE_ERR -1
#define PACKAGE_MORE 0
#define PACKAGE_DONE 2

typedef struct PackageParser {
    // Maximum complete plaintext size, including framing; set before parsing.
    size_t max_size;
    // Partial header or size field, retained when an input chunk ends mid-field.
    uint8_t field[8];
    size_t field_size;
    uint32_t blob_index;
    size_t blob_offset;
    enum {
        PACKAGE_HEADER,
        PACKAGE_SIZES,
        PACKAGE_DATA,
        PACKAGE_END,
        PACKAGE_FAILED
    } state;
} PackageParser;

/* V1 plaintext: version:u16, count:u32, all sizes:u64, then all payloads in order.
 * Integers are big-endian; count is nonzero and empty blobs are allowed.
 * Start with an otherwise zeroed parser whose max_size is the caller's byte limit,
 * and a zeroed staging Package. Keep both objects for the entire stream.
 * Each call copies input chunks into owned storage; input must not overlap that storage.
 * Descriptors are allocated after the header and one buffer after the complete size table.
 * Return MORE, DONE, or ERR. Call finish_package() at the actual end of input.
 * ERR is terminal. Destroy staging contents on failure; reset the parser before reuse.
 * Contents remain provisional until parsing and authentication finish.
 */
int parse_package_contents(
    PackageParser* parser,
    Package* package,
    const uint8_t* bytes,
    size_t size
);

int finish_package(PackageParser* parser);

typedef struct PackageMarshaler {
    size_t offset;       // Bytes emitted across the complete wire package.
    uint32_t blob_index; // Current payload; unused while writing the size table.
    size_t blob_offset;  // Bytes emitted within that payload.
} PackageMarshaler;

/* Start with a zeroed marshaler and keep the Package unchanged until DONE.
 * Fill a caller-owned output chunk without allocation; return MORE, DONE, or ERR.
 * Output must not overlap the package storage or marshaler.
 * Invalid input leaves output, written, and progress unchanged.
 */
int marshal_package(
    PackageMarshaler* marshaler,
    const Package* package,
    uint8_t* bytes,
    size_t capacity,
    size_t* written
);
