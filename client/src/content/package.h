/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct PackageBlob {
    size_t size;
    // Borrowed view into the owning Package buffer; never free this pointer.
    uint8_t* bytes;
} PackageBlob;

typedef struct Package {
    // Array positions identify blobs; blob zero contains the post data.
    PackageBlob* blobs;
    uint32_t blob_count;
    uint8_t* bytes;
    size_t size;
} Package;

// Allocate fresh descriptors and one fixed-size plaintext buffer; return 0 or -1.
// Populate blob views separately. Destroy a live package before reinitializing it.
int package_init(Package* package, uint32_t blob_count, size_t size);

// Wipe and release the shared buffer, then release all descriptors.
void package_destroy(Package* package);
