/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "codec/encrypted_blob_reader.h"
#include <sodium.h>
#include <assert.h>

static const Key key = {{1, 2, 3}};

static size_t make_blob(uint8_t* wire) {
    BlobCrypt writer = {0};
    assert(encrypt_blob_header(&writer, wire, &key) == 0);
    uint8_t chunk[32] = "first";
    Buffer buffer = {chunk, sizeof(chunk), 5};
    assert(encrypt_blob_chunk(&writer, &buffer, false) == 0);
    size_t size = ENCRYPTED_BLOB_HEADER_SIZE;
    memcpy(wire + size, chunk, buffer.size);
    size += buffer.size;

    memcpy(chunk, "last", 4);
    buffer.size = 4;
    assert(encrypt_blob_chunk(&writer, &buffer, true) == 0);
    memcpy(wire + size, chunk, buffer.size);
    return size + buffer.size;
}

static void fragmented(size_t fragment_size) {
    uint8_t wire[128], scratch[32], output[9];
    size_t size = make_blob(wire);
    EncryptedBlobReader reader = {.remaining = size};
    Buffer buffer = {scratch, sizeof(scratch), 0};
    size_t produced = 0;

    for (size_t offset = 0; offset < size;) {
        uint8_t borrowed[128];
        size_t length = size - offset;
        if (length > fragment_size) {
            length = fragment_size;
        }
        memcpy(borrowed, wire + offset, length);

        for (size_t position = 0; position < length;) {
            size_t consumed;
            int result = parse_encrypted_blob(
                &reader, &key, &buffer, borrowed + position, length - position, &consumed
            );
            assert(result != ENCRYPTED_BLOB_ERR && consumed > 0);
            position += consumed;
            if (result == ENCRYPTED_BLOB_MORE) {
                assert(buffer.size == 0);
            } else {
                assert(produced + buffer.size <= sizeof(output));
                memcpy(output + produced, buffer.b, buffer.size);
                produced += buffer.size;
                assert(result == (position + offset == size
                    ? ENCRYPTED_BLOB_DONE : ENCRYPTED_BLOB_CHUNK));
            }
        }
        memset(borrowed, 0xff, sizeof(borrowed));
        offset += length;
    }

    assert(produced == sizeof(output) && memcmp(output, "firstlast", sizeof(output)) == 0);
    assert(finish_encrypted_blob(&reader) == ENCRYPTED_BLOB_DONE);
    assert(reader.crypt.crypt.mode == CRYPT_CLOSED);
}

static void malformed(void) {
    uint8_t wire[128], scratch[32];
    size_t size = make_blob(wire);

    // Actual EOF can arrive anywhere, even with a larger declared size.
    for (size_t cutoff = 0; cutoff < size; cutoff++) {
        EncryptedBlobReader reader = {.remaining = size};
        Buffer buffer = {scratch, sizeof(scratch), 0};
        size_t offset = 0;
        while (offset < cutoff) {
            size_t consumed;
            int result = parse_encrypted_blob(
                &reader, &key, &buffer, wire + offset, cutoff - offset, &consumed
            );
            assert(result != ENCRYPTED_BLOB_ERR && consumed > 0);
            offset += consumed;
        }
        assert(finish_encrypted_blob(&reader) == ENCRYPTED_BLOB_ERR);
        assert(reader.crypt.crypt.mode == CRYPT_CLOSED);
    }

    // Corrupt a length, ciphertext byte, or declared total; nothing can be committed.
    for (size_t change = 0; change < 5; change++) {
        uint8_t changed[128];
        memcpy(changed, wire, size);
        uint64_t declared = size;
        if (change == 0) {
            changed[ENCRYPTED_BLOB_HEADER_SIZE] = 0xff;
        } else if (change == 1) {
            changed[ENCRYPTED_BLOB_HEADER_SIZE + ENCRYPTED_CHUNK_PREFIX_SIZE] ^= 1;
        } else if (change == 2) {
            declared--;
        } else if (change == 3) {
            declared += ENCRYPTED_CHUNK_OVERHEAD;
        }
        EncryptedBlobReader reader = {.remaining = declared};
        Buffer buffer = {scratch, change == 4 ? 8 : sizeof(scratch), 0};
        int result = ENCRYPTED_BLOB_MORE;
        for (size_t offset = 0; offset < size && result != ENCRYPTED_BLOB_ERR;) {
            size_t consumed;
            result = parse_encrypted_blob(
                &reader, &key, &buffer, changed + offset, size - offset, &consumed
            );
            offset += consumed;
        }
        assert(result == ENCRYPTED_BLOB_ERR && buffer.size == 0);
        assert(sodium_is_zero(scratch, buffer.cap));
        assert(finish_encrypted_blob(&reader) == ENCRYPTED_BLOB_ERR);
    }

    EncryptedBlobReader reader = {.remaining = size};
    Buffer buffer = {scratch, sizeof(scratch), 0};
    size_t offset = 0;
    while (offset < size) {
        size_t consumed;
        assert(parse_encrypted_blob(&reader, &key, &buffer,
            wire + offset, size - offset, &consumed) > ENCRYPTED_BLOB_MORE);
        offset += consumed;
    }
    size_t consumed;
    assert(parse_encrypted_blob(&reader, &key, &buffer, wire, 1, &consumed)
        == ENCRYPTED_BLOB_ERR);
    assert(buffer.size == 0 && reader.crypt.crypt.mode == CRYPT_CLOSED);
}

int main(void) {
    assert(sodium_init() >= 0);
    for (size_t fragment_size = 1; fragment_size <= 128; fragment_size++) {
        fragmented(fragment_size);
    }
    malformed();
    return 0;
}
