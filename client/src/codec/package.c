/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "codec/package.h"
#include <string.h>

static uint64_t read_uint(const uint8_t* bytes, size_t size) {
    uint64_t value = 0;
    for (size_t i = 0; i < size; i++) {
        value = (value << 8) | bytes[i];
    }
    return value;
}

static int reject(PackageParser* parser) {
    if (parser) parser->state = PACKAGE_FAILED;
    return PACKAGE_ERR;
}

int parse_package(
    PackageParser* parser,
    const uint8_t* bytes,
    size_t size,
    size_t* consumed,
    PackageSlice* blob
) {
    if (!parser || !consumed || !blob || (size && !bytes)) {
        return reject(parser);
    }
    *consumed = 0;
    memset(blob, 0, sizeof(*blob));

    for (;;) {
        if (parser->state == PACKAGE_FAILED) return PACKAGE_ERR;
        if (parser->state == PACKAGE_END) {
            return *consumed == size ? PACKAGE_DONE : reject(parser);
        }

        if (parser->state == PACKAGE_HEADER || parser->state == PACKAGE_LENGTH) {
            size_t needed = parser->state == PACKAGE_HEADER ? 6 : 8;
            size_t take = needed - parser->field_size;
            if (take > size - *consumed) take = size - *consumed;
            if (take) {
                memcpy(parser->field + parser->field_size, bytes + *consumed, take);
            }
            parser->field_size += take;
            *consumed += take;
            if (parser->field_size < needed) return PACKAGE_MORE;
            parser->field_size = 0;

            if (parser->state == PACKAGE_HEADER) {
                if (read_uint(parser->field, 2) != 1) return reject(parser);
                parser->blob_count = (uint32_t)read_uint(parser->field + 2, 4);
                if (!parser->blob_count) return reject(parser);
                parser->state = PACKAGE_LENGTH;
                continue;
            }
            parser->blob_size = read_uint(parser->field, 8);
            parser->blob_offset = 0;
            parser->state = PACKAGE_DATA;
        }

        uint64_t remaining = parser->blob_size - parser->blob_offset;
        size_t take = size - *consumed;
        if (remaining < take) take = (size_t)remaining;
        if (!take && remaining) return PACKAGE_MORE;

        // An empty blob still emits a slice, so callers can preserve its position.
        *blob = (PackageSlice){
            parser->blob_index, parser->blob_size, parser->blob_offset,
            take ? bytes + *consumed : NULL, take
        };
        parser->blob_offset += take;
        *consumed += take;
        if (parser->blob_offset == parser->blob_size) {
            parser->blob_index++;
            parser->state = parser->blob_index == parser->blob_count ? PACKAGE_END : PACKAGE_LENGTH;
        }
        return PACKAGE_BLOB;
    }
}

int finish_package(PackageParser* parser) {
    return parser && parser->state == PACKAGE_END ? PACKAGE_DONE : reject(parser);
}
