/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "content/package.h"
#include "sz_common/codec.h"

// Complete encrypted envelope size, including package framing; zero means invalid or over the package limit.
size_t package_ciphertext_size(const Package* package);

// Prepare one in-memory ciphertext package using authenticated chunks of at most 64 KiB.
// Caller owns output and supplies package_ciphertext_size() bytes of capacity.
// Output must not overlap the package or key. Source remains unchanged.
// Success returns 0 and the hash of the complete ciphertext, including envelope/framing.
// Failure returns -1; partially prepared output is wiped. Retain ciphertext and key for upload.
int encrypt_package(
    const Package* package, const Key* key,
    uint8_t* output, size_t capacity, HashT* hash
);
