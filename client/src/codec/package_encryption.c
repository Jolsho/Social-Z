/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "codec/package_encryption.h"
#include "codec/package.h"
#include "codec/encrypted_blob.h"
#include "sz_common/hash.h"
#include "sz_client/limits.h"
#include <sodium.h>

size_t package_ciphertext_size(const Package* package) {
    if (!package || !package->blob_count || !package->blobs) {
        return 0;
    }

    size_t count = package->blob_count;
    if (count > (SIZE_MAX - 6) / 8) {
        return 0;
    }

    // Plaintext starts with version:u16, count:u32, and one size:u64 per blob.
    size_t framing = 6 + count * 8;
    if (package->size > SIZE_MAX - framing) {
        return 0;
    }
    size_t plaintext = framing + package->size;
    size_t chunks = plaintext / CRYPT_RECORD_MAX + (plaintext % CRYPT_RECORD_MAX != 0);
    if (chunks > (SIZE_MAX - ENCRYPTED_BLOB_HEADER_SIZE) / ENCRYPTED_CHUNK_OVERHEAD) {
        return 0;
    }
    size_t overhead = ENCRYPTED_BLOB_HEADER_SIZE + chunks * ENCRYPTED_CHUNK_OVERHEAD;
    if (plaintext > SIZE_MAX - overhead) {
        return 0;
    }
    size_t total = plaintext + overhead;
    return total <= SZ_PACKAGE_CIPHERTEXT_MAX ? total : 0;
}

int encrypt_package(
    const Package* package, const Key* key,
    uint8_t* output, size_t capacity, HashT* hash
) {
    size_t total = package_ciphertext_size(package);
    if (!total || !key || !output || !hash || capacity < total) {
        return -1;
    }

    BlobCrypt encryption = {0};
    PackageMarshaler marshaler = {0};
    Hasher hasher = new_hasher();
    if (encrypt_blob_header(&encryption, output, key) != 0) {
        return -1;
    }
    size_t used = ENCRYPTED_BLOB_HEADER_SIZE;
    hash_update(&hasher, output, used);

    // Marshal directly into each ciphertext region, then encrypt that region in place.
    // The finished allocation can be sent later without retaining plaintext or crypto state.
    int result;
    do {
        size_t chunk_capacity = total - used;
        if (chunk_capacity > CRYPT_RECORD_MAX + ENCRYPTED_CHUNK_OVERHEAD) {
            chunk_capacity = CRYPT_RECORD_MAX + ENCRYPTED_CHUNK_OVERHEAD;
        }
        Buffer chunk = {.b = output + used, .cap = (uint32_t)chunk_capacity};
        size_t written;
        result = marshal_package(
            &marshaler, package, chunk.b,
            chunk.cap - ENCRYPTED_CHUNK_OVERHEAD, &written
        );
        if (result == PACKAGE_ERR) {
            goto failed;
        }
        chunk.size = (uint32_t)written;
        if (encrypt_blob_chunk(&encryption, &chunk, result == PACKAGE_DONE) != 0) {
            goto failed;
        }
        hash_update(&hasher, chunk.b, chunk.size);
        used += chunk.size;
    } while (result == PACKAGE_MORE);

    *hash = hash_finalize(&hasher);
    return 0;

failed:
    crypt_clear(&encryption.crypt);
    sodium_memzero(output, total);
    return -1;
}
