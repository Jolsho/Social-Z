/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "codec/encrypted_blob.h"
#include "codec/package.h"
#include <sodium.h>
#include <assert.h>

static const Key key = {{1, 2, 3}};

static void round_trip(size_t size) {
    uint8_t bytes[CRYPT_RECORD_MAX + ENCRYPTED_CHUNK_OVERHEAD];
    for (size_t i = 0; i < size; i++) {
        bytes[i] = (uint8_t)i;
    }
    Buffer buffer = {bytes, sizeof(bytes), size};
    BlobCrypt writer = {0}, reader = {0};
    uint8_t header[ENCRYPTED_BLOB_HEADER_SIZE];
    assert(encrypt_blob_header(&writer, header, &key) == 0);
    assert(header[0] == 0 && header[1] == 1 && header[2] == 0 && header[3] == 1);
    assert(encrypt_blob_chunk(&writer, &buffer, true) == 0);
    assert(buffer.b == bytes && buffer.size == size + ENCRYPTED_CHUNK_OVERHEAD);
    uint32_t cipher_size = size + CRYPT_RECORD_OVERHEAD;
    for (size_t i = 0; i < 4; i++) {
        assert(bytes[i] == (uint8_t)(cipher_size >> (8 * (3 - i))));
    }

    assert(decrypt_blob_header(&reader, header, sizeof(header), &key) == 0);
    assert(decrypt_blob_chunk(&reader, &buffer, true) == 0);
    assert(buffer.b == bytes && buffer.size == size);
    for (size_t i = 0; i < size; i++) {
        assert(bytes[i] == (uint8_t)i);
    }
    assert(sodium_is_zero(bytes + size, ENCRYPTED_CHUNK_OVERHEAD));
    assert(writer.crypt.mode == CRYPT_CLOSED && reader.crypt.mode == CRYPT_CLOSED);
    assert(decrypt_blob_chunk(&reader, &buffer, true) == -1);
}

static void failures(void) {
    uint8_t saved[32] = "abc", header[ENCRYPTED_BLOB_HEADER_SIZE];
    BlobCrypt writer = {0};
    assert(encrypt_blob_header(&writer, header, &key) == 0);
    Buffer buffer = {saved, sizeof(saved), 3};
    buffer.cap = 3 + ENCRYPTED_CHUNK_OVERHEAD - 1;
    BlobCrypt before = writer;
    assert(encrypt_blob_chunk(&writer, &buffer, true) == -1);
    assert(memcmp(saved, "abc", 3) == 0 && buffer.size == 3);
    assert(memcmp(&writer, &before, sizeof(writer)) == 0);
    buffer.cap = sizeof(saved);
    assert(encrypt_blob_chunk(&writer, &buffer, true) == 0);
    size_t size = buffer.size;

    for (size_t i = 0; i < size + ENCRYPTED_BLOB_HEADER_SIZE + 3; i++) {
        uint8_t bytes[sizeof(saved)], envelope[sizeof(header)];
        memcpy(bytes, saved, size);
        memcpy(envelope, header, sizeof(header));
        BlobCrypt reader = {0};
        Key changed_key = key;
        if (i < size) {
            bytes[i] ^= 1;
        } else if (i < size + sizeof(header)) {
            envelope[i - size] ^= 1;
        } else if (i == size + sizeof(header)) {
            changed_key.b[0] ^= 1;
        }

        int result = decrypt_blob_header(&reader, envelope, sizeof(envelope), &changed_key);
        if (result == 0) {
            if (i == size + sizeof(header) + 1) {
                reader.chunk_number++;
            }
            bool final = i != size + sizeof(header) + 2;
            buffer = (Buffer){bytes, sizeof(bytes), size};
            assert(decrypt_blob_chunk(&reader, &buffer, final) == -1);
            // Invalid framing is rejected before decryption; authentication failures wipe.
            if (i >= ENCRYPTED_CHUNK_PREFIX_SIZE) {
                assert(buffer.size == 0 && sodium_is_zero(bytes, size));
                assert(reader.crypt.mode == CRYPT_CLOSED);
            }
        }
        crypt_clear(&reader.crypt);
    }

    BlobCrypt reader = {0};
    assert(decrypt_blob_header(&reader, header, sizeof(header) - 1, &key) == -1);
    assert(decrypt_blob_header(&reader, header, sizeof(header), &key) == 0);
    for (size_t truncated = 0; truncated < size; truncated++) {
        uint8_t bytes[sizeof(saved)];
        memcpy(bytes, saved, size);
        buffer = (Buffer){bytes, sizeof(bytes), truncated};
        assert(decrypt_blob_chunk(&reader, &buffer, true) == -1);
        assert(memcmp(bytes, saved, size) == 0);
    }
    buffer = (Buffer){saved, sizeof(saved), size + 1};
    assert(decrypt_blob_chunk(&reader, &buffer, true) == -1);
    crypt_clear(&reader.crypt);
}

static void package_chunks(void) {
    // Marshal, encrypt, and decrypt successive package slices in one networking allocation.
    uint8_t data[] = "text and attachment";
    PackageBlob blobs[] = {
        {.size = 4, .bytes = data},
        {.size = sizeof(data) - 4, .bytes = data + 4},
    };
    Package package = {
        .blobs = blobs, .blob_count = 2, .bytes = data, .size = sizeof(data),
    };
    Package staging = {0};
    PackageParser parser = {.max_size = 128};
    PackageMarshaler marshaler = {0};
    BlobCrypt writer = {0}, reader = {0};
    uint8_t header[ENCRYPTED_BLOB_HEADER_SIZE], bytes[8 + ENCRYPTED_CHUNK_OVERHEAD];
    assert(encrypt_blob_header(&writer, header, &key) == 0);
    assert(decrypt_blob_header(&reader, header, sizeof(header), &key) == 0);

    int result;
    do {
        size_t written;
        result = marshal_package(&marshaler, &package, bytes, 8, &written);
        assert(result == PACKAGE_MORE || result == PACKAGE_DONE);
        bool final = result == PACKAGE_DONE;
        Buffer buffer = {bytes, sizeof(bytes), written};
        assert(encrypt_blob_chunk(&writer, &buffer, final) == 0);
        assert(decrypt_blob_chunk(&reader, &buffer, final) == 0);
        assert(parse_package_contents(&parser, &staging, buffer.b, buffer.size)
            == (final ? PACKAGE_DONE : PACKAGE_MORE));
        memset(bytes, 0, sizeof(bytes));
    } while (result != PACKAGE_DONE);

    assert(finish_package(&parser) == PACKAGE_DONE);
    assert(staging.blob_count == 2);
    assert(memcmp(staging.blobs[0].bytes, data, 4) == 0);
    assert(memcmp(staging.blobs[1].bytes, data + 4, sizeof(data) - 4) == 0);
    package_destroy(&staging);
}

int main(void) {
    assert(sodium_init() >= 0);
    round_trip(0);
    round_trip(1);
    round_trip(CRYPT_RECORD_MAX);
    failures();
    package_chunks();
    return 0;
}
