/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "codec/package_encryption.h"
#include "codec/package.h"
#include "codec/encrypted_blob.h"
#include "sz_common/hash.h"
#include "sz_client/limits.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void round_trip(size_t payload_size) {
    Package source = {0}, decoded = {0};
    assert(package_init(&source, 3, payload_size) == 0);
    source.blobs[0] = (PackageBlob){.size = payload_size / 3, .bytes = source.bytes};
    source.blobs[1] = (PackageBlob){.size = 0};
    source.blobs[2] = (PackageBlob){
        .size = payload_size - source.blobs[0].size,
        .bytes = source.bytes ? source.bytes + source.blobs[0].size : NULL,
    };
    for (size_t i = 0; i < payload_size; i++) {
        source.bytes[i] = (uint8_t)i;
    }

    Key key = {{7}};
    size_t size = package_ciphertext_size(&source);
    uint8_t* ciphertext = malloc(size);
    assert(ciphertext);
    HashT hash;
    assert(encrypt_package(&source, &key, ciphertext, size, &hash) == 0);
    Hasher hasher = new_hasher();
    hash_update(&hasher, ciphertext, size);
    HashT actual = hash_finalize(&hasher);
    assert(memcmp(hash.b, actual.b, HASH_SIZE) == 0);

    BlobCrypt decryptor;
    assert(decrypt_blob_header(&decryptor, ciphertext, ENCRYPTED_BLOB_HEADER_SIZE, &key) == 0);
    PackageParser parser = {.max_size = 6 + 3 * 8 + payload_size};
    size_t offset = ENCRYPTED_BLOB_HEADER_SIZE;
    size_t chunks = 0;
    while (offset < size) {
        uint32_t length = 0;
        for (size_t i = 0; i < 4; i++) {
            length = (length << 8) | ciphertext[offset + i];
        }
        size_t framed = 4 + length;
        assert(framed <= size - offset);
        bool final = offset + framed == size;
        Buffer chunk = {
            .b = ciphertext + offset, .cap = (uint32_t)framed, .size = (uint32_t)framed,
        };
        assert(decrypt_blob_chunk(&decryptor, &chunk, final) == 0);
        assert(parse_package_contents(&parser, &decoded, chunk.b, chunk.size) ==
            (final ? PACKAGE_DONE : PACKAGE_MORE));
        offset += framed;
        chunks++;
    }
    assert(finish_package(&parser) == PACKAGE_DONE);
    assert(chunks == (parser.max_size + CRYPT_RECORD_MAX - 1) / CRYPT_RECORD_MAX);
    assert(decoded.blob_count == 3 && decoded.size == payload_size);
    assert(decoded.blobs[0].size == source.blobs[0].size && !decoded.blobs[1].size);
    if (payload_size) {
        assert(memcmp(decoded.bytes, source.bytes, payload_size) == 0);
    }
    package_destroy(&decoded);
    package_destroy(&source);
    free(ciphertext);
}

static void rejected_input(void) {
    Package package = {0};
    assert(package_init(&package, 1, 1) == 0);
    package.blobs[0] = (PackageBlob){.size = 1, .bytes = package.bytes};
    size_t size = package_ciphertext_size(&package);
    uint8_t* output = malloc(size);
    assert(output);
    Key key = {{3}};
    HashT hash;
    memset(output, 0xa5, size);
    memset(&hash, 0xa5, sizeof(hash));
    assert(encrypt_package(&package, &key, output, size - 1, &hash) == -1);
    assert(output[0] == 0xa5 && hash.b[0] == 0xa5);

    // Invalid payload descriptors must not leave a usable partial envelope behind.
    package.blobs[0].bytes = NULL;
    assert(encrypt_package(&package, &key, output, size, &hash) == -1);
    for (size_t i = 0; i < size; i++) {
        assert(output[i] == 0);
    }
    assert(hash.b[0] == 0xa5);
    package.size = SIZE_MAX;
    assert(!package_ciphertext_size(&package));
    package.size = 1;
    package_destroy(&package);
    free(output);
}

static void size_limit(void) {
    PackageBlob blob = {0};
    // Metadata-only check: no large allocation is needed to exercise the exact limit.
    size_t chunks = SZ_PACKAGE_CIPHERTEXT_MAX / CRYPT_RECORD_MAX;
    Package package = {
        .blobs = &blob, .blob_count = 1,
        .size = SZ_PACKAGE_CIPHERTEXT_MAX - ENCRYPTED_BLOB_HEADER_SIZE
            - chunks * ENCRYPTED_CHUNK_OVERHEAD - 14,
    };
    assert(package_ciphertext_size(&package) == SZ_PACKAGE_CIPHERTEXT_MAX);
    package.size++;
    assert(package_ciphertext_size(&package) == 0);
}

int main(void) {
    round_trip(0);
    round_trip(100);
    round_trip(CRYPT_RECORD_MAX - 30); // Exact full chunk, including plaintext framing.
    round_trip(CRYPT_RECORD_MAX - 29); // One byte spills into a final second chunk.
    round_trip(3 * CRYPT_RECORD_MAX + 17);
    rejected_input();
    size_limit();
    return 0;
}
