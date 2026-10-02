/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#undef calloc
#undef free
#include "content/package.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sodium.h>

static size_t allocation_count, fail_at;
static uint8_t* watched_bytes;
static size_t watched_size;

void* package_test_calloc(size_t count, size_t size) {
    return ++allocation_count == fail_at ? NULL : calloc(count, size);
}

void package_test_free(void* bytes) {
    if (bytes && bytes == watched_bytes) {
        assert(sodium_is_zero(bytes, watched_size));
        watched_bytes = NULL;
    }
    free(bytes);
}

static void shared_storage(void) {
    Package package = {0};
    assert(package_init(&package, 3, 7) == 0);
    assert(package.blob_count == 3 && package.size == 7);
    assert(sodium_is_zero(package.bytes, package.size));
    for (uint32_t i = 0; i < package.blob_count; i++) {
        assert(package.blobs[i].bytes == NULL && package.blobs[i].size == 0);
    }

    const uint8_t contents[] = {'a', 'b', 'c', 0, 0xff, 1, 2};
    memcpy(package.bytes, contents, sizeof(contents));
    package.blobs[0] = (PackageBlob){3, package.bytes};
    package.blobs[1] = (PackageBlob){0, package.bytes + 3};
    package.blobs[2] = (PackageBlob){4, package.bytes + 3};
    assert(memcmp(package.blobs[0].bytes, "abc", 3) == 0);
    assert(memcmp(package.blobs[2].bytes, contents + 3, 4) == 0);
    package.blobs[2].bytes[0] = 42;
    assert(package.bytes[3] == 42);

    watched_bytes = package.bytes;
    watched_size = package.size;
    package_destroy(&package);
    assert(watched_bytes == NULL);
    assert(package.bytes == NULL && package.size == 0);
    assert(package.blobs == NULL && package.blob_count == 0);
    package_destroy(&package);
}

static void empty_and_failed_setup(void) {
    Package package = {0};
    assert(package_init(NULL, 1, 7) == -1);
    assert(package_init(&package, 0, 7) == -1);
    for (size_t failure = 1; failure <= 2; failure++) {
        allocation_count = 0;
        fail_at = failure;
        assert(package_init(&package, 1, 7) == -1);
        assert(package.blobs == NULL && package.blob_count == 0);
        assert(package.bytes == NULL && package.size == 0);
        package_destroy(&package);
    }
    fail_at = 0;
    assert(package_init(&package, 1, 0) == 0);
    assert(package.blobs && package.bytes == NULL && package.size == 0);
    package_destroy(&package);
    package_destroy(NULL);
}

int main(void) {
    shared_storage();
    empty_and_failed_setup();
    return 0;
}
