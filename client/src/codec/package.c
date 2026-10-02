/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "codec/package.h"
#include <stdlib.h>
#include <string.h>

enum {
    PACKAGE_HEADER_BYTES = 6,    // version:u16 + blob_count:u32
    PACKAGE_BLOB_SIZE_BYTES = 8  // one big-endian u64 length
};

static uint64_t read_uint(const uint8_t* bytes, size_t size) {
    uint64_t value = 0;

    for (size_t i = 0; i < size; i++) {
        value = (value << 8) | bytes[i];
    }

    return value;
}

static int reject(PackageParser* parser) {
    if (parser) {
        parser->state = PACKAGE_FAILED;
    }

    return PACKAGE_ERR;
}

int parse_package_contents(
    PackageParser* parser,
    Package* package,
    const uint8_t* bytes,
    size_t size
) {
    if (!parser || !package || (size && !bytes)) {
        return reject(parser);
    }

    size_t consumed = 0;

    for (;;) {
        if (parser->state == PACKAGE_FAILED) {
            return PACKAGE_ERR;
        }
        if (parser->state == PACKAGE_END) {
            return consumed == size ? PACKAGE_DONE : reject(parser);
        }

        // Framing fields can straddle input chunks; retain only their partial bytes.
        if (parser->state == PACKAGE_HEADER || parser->state == PACKAGE_SIZES) {
            size_t needed = parser->state == PACKAGE_HEADER
                ? PACKAGE_HEADER_BYTES
                : PACKAGE_BLOB_SIZE_BYTES;
            size_t take = needed - parser->field_size;
            if (take > size - consumed) {
                take = size - consumed;
            }

            if (take) {
                memcpy(parser->field + parser->field_size, bytes + consumed, take);
            }
            parser->field_size += take;
            consumed += take;

            if (parser->field_size < needed) {
                return PACKAGE_MORE;
            }
            parser->field_size = 0;

            if (parser->state == PACKAGE_HEADER) {
                // The first two bytes are the version; the next four are the blob count.
                uint16_t version = (uint16_t)read_uint(parser->field, 2);
                uint32_t count = (uint32_t)read_uint(parser->field + 2, 4);
                if (version != 1 || !count || parser->max_size < PACKAGE_HEADER_BYTES) {
                    return reject(parser);
                }

                // Bound the descriptor count by the minimum bytes its size table requires.
                size_t max_count =
                    (parser->max_size - PACKAGE_HEADER_BYTES) / PACKAGE_BLOB_SIZE_BYTES;
                if (count > max_count) {
                    return reject(parser);
                }
                if (package->blobs || package->bytes || package_init(package, count, 0) != 0) {
                    return reject(parser);
                }

                parser->state = PACKAGE_SIZES;
                continue;
            }

            uint64_t length = read_uint(parser->field, PACKAGE_BLOB_SIZE_BYTES);
            size_t table_bytes = (size_t)package->blob_count * PACKAGE_BLOB_SIZE_BYTES;
            size_t payload_limit = parser->max_size - PACKAGE_HEADER_BYTES - table_bytes;
            if (length > payload_limit - package->size) {
                return reject(parser);
            }
            package->blobs[parser->blob_index].size = (size_t)length;
            parser->blob_index++;
            package->size += (size_t)length;

            if (parser->blob_index < package->blob_count) {
                continue;
            }

            // The full size table is known; allocate one buffer and assign stable blob views.
            if (package->size) {
                package->bytes = calloc(package->size, 1);
                if (!package->bytes) {
                    return reject(parser);
                }
            }

            size_t offset = 0;
            for (uint32_t i = 0; i < package->blob_count; i++) {
                package->blobs[i].bytes = package->bytes ? package->bytes + offset : NULL;
                offset += package->blobs[i].size;
            }

            parser->blob_index = 0;
            parser->state = PACKAGE_DATA;
        }

        PackageBlob* blob = &package->blobs[parser->blob_index];
        size_t remaining = blob->size - parser->blob_offset;
        size_t take = size - consumed;
        if (take > remaining) {
            take = remaining;
        }
        if (!take && remaining) {
            return PACKAGE_MORE;
        }

        // Copy this payload slice now; the caller may reuse its input buffer after return.
        if (take) {
            memcpy(blob->bytes + parser->blob_offset, bytes + consumed, take);
        }
        consumed += take;
        parser->blob_offset += take;

        if (parser->blob_offset == blob->size) {
            parser->blob_offset = 0;
            parser->blob_index++;
            if (parser->blob_index == package->blob_count) {
                parser->state = PACKAGE_END;
            }
        }
    }
}

int finish_package(PackageParser* parser) {
    return parser && parser->state == PACKAGE_END ? PACKAGE_DONE : reject(parser);
}
