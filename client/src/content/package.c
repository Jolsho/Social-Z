/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "content/package.h"
#include <stdlib.h>
#include <string.h>
#include <sodium.h>

int package_init(Package* package, uint32_t blob_count, size_t size) {
    if (!package) {
        return -1;
    }

    memset(package, 0, sizeof(*package));
    size_t count = blob_count;
    if (!count || count > SIZE_MAX / sizeof(PackageBlob)) {
        return -1;
    }

    Package contents = {0};
    contents.blobs = calloc(count, sizeof(PackageBlob));
    if (!contents.blobs) {
        return -1;
    }
    if (size) {
        contents.bytes = calloc(size, 1);
        if (!contents.bytes) {
            free(contents.blobs);
            return -1;
        }
    }
    contents.blob_count = blob_count;
    contents.size = size;
    *package = contents;
    return 0;
}

void package_destroy(Package* package) {
    if (!package) {
        return;
    }

    if (package->bytes) {
        sodium_memzero(package->bytes, package->size);
        free(package->bytes);
    }
    free(package->blobs);
    memset(package, 0, sizeof(*package));
}
