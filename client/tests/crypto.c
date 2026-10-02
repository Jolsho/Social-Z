/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "utils/crypto.h"
#include "codec/feed.h"
#include <sodium.h>
#include <assert.h>

static const Key key = {{1, 2, 3}};
static const uint8_t ad[] = {0, 1, 0, 1, 0, 0, 0, 7};

static void round_trip(size_t size) {
    static uint8_t bytes[CRYPT_RECORD_MAX + CRYPT_RECORD_OVERHEAD];
    uint8_t header[CRYPT_HEADER_SIZE], reference[sizeof(bytes)];
    for (size_t i = 0; i < size; i++) {
        bytes[i] = (uint8_t)i;
    }

    Buffer buffer = {bytes, size + CRYPT_RECORD_OVERHEAD, size};
    CryptCtx writer = {0}, reader = {0};
    assert(crypt_init_encrypt(&writer, header, &key) == 0);

    // Compare overlapping encryption with a separate libsodium output buffer.
    crypto_secretstream_xchacha20poly1305_state independent = writer.stream;
    assert(crypto_secretstream_xchacha20poly1305_push(&independent, reference, NULL,
        bytes, size, ad, sizeof(ad), crypto_secretstream_xchacha20poly1305_TAG_FINAL) == 0);

    assert(encrypt(&writer, &buffer, ad, sizeof(ad), true) == 0);
    assert(buffer.b == bytes && buffer.size == size + CRYPT_RECORD_OVERHEAD);
    assert(memcmp(bytes, reference, buffer.size) == 0);
    assert(sodium_is_zero((const uint8_t*)&writer, sizeof(writer)));
    assert(encrypt(&writer, &buffer, ad, sizeof(ad), true) == -1);

    assert(crypt_init_decrypt(&reader, header, &key) == 0);
    assert(decrypt(&reader, &buffer, ad, sizeof(ad), true) == 0);
    assert(buffer.b == bytes && buffer.size == size);
    for (size_t i = 0; i < size; i++) {
        assert(bytes[i] == (uint8_t)i);
    }
    assert(sodium_is_zero(bytes + size, CRYPT_RECORD_OVERHEAD));
    assert(sodium_is_zero((const uint8_t*)&reader, sizeof(reader)));
    assert(decrypt(&reader, &buffer, ad, sizeof(ad), true) == -1);
}

static void capacity_and_arguments(void) {
    uint8_t bytes[64] = {1, 2, 3}, before[sizeof(bytes)], header[CRYPT_HEADER_SIZE];
    memcpy(before, bytes, sizeof(bytes));
    Buffer buffer = {bytes, 3 + CRYPT_RECORD_OVERHEAD - 1, 3};
    CryptCtx writer = {0};
    assert(crypt_init_encrypt(&writer, header, &key) == 0);
    CryptCtx state = writer;

    assert(encrypt(&writer, &buffer, ad, sizeof(ad), true) == -1);
    assert(buffer.size == 3 && memcmp(bytes, before, sizeof(bytes)) == 0);
    assert(memcmp(&writer, &state, sizeof(state)) == 0);
    buffer.cap++;
    assert(encrypt(&writer, &buffer, NULL, 1, true) == -1);
    assert(encrypt(NULL, &buffer, ad, sizeof(ad), true) == -1);
    assert(encrypt(&writer, NULL, ad, sizeof(ad), true) == -1);
    assert(buffer.size == 3 && memcmp(bytes, before, sizeof(bytes)) == 0);

    Buffer invalid = {bytes, 1, 2};
    assert(encrypt(&writer, &invalid, NULL, 0, true) == -1);
    invalid = (Buffer){bytes, UINT32_MAX, CRYPT_RECORD_MAX + 1};
    assert(encrypt(&writer, &invalid, NULL, 0, true) == -1);
    assert(encrypt(&writer, &buffer, NULL, 0, true) == 0);

    CryptCtx reader = {0};
    assert(crypt_init_decrypt(&reader, header, &key) == 0);
    invalid = (Buffer){bytes, sizeof(bytes), CRYPT_RECORD_OVERHEAD - 1};
    assert(decrypt(&reader, &invalid, NULL, 0, true) == -1);
    invalid = (Buffer){bytes, 1, 2};
    assert(decrypt(&reader, &invalid, NULL, 0, true) == -1);
    invalid = (Buffer){bytes, UINT32_MAX, CRYPT_RECORD_MAX + CRYPT_RECORD_OVERHEAD + 1};
    assert(decrypt(&reader, &invalid, NULL, 0, true) == -1);
    assert(decrypt(&reader, &buffer, NULL, 0, true) == 0);
    assert(buffer.size == 3 && memcmp(bytes, before, 3) == 0);
    assert(crypt_init_encrypt(NULL, header, &key) == -1);
    assert(crypt_init_decrypt(&reader, NULL, &key) == -1);
    crypt_clear(&reader);
    crypt_clear(NULL);
}

static void failures(void) {
    uint8_t saved[32], header[CRYPT_HEADER_SIZE];
    Buffer buffer = {saved, sizeof(saved), 3};
    memcpy(saved, "abc", 3);
    CryptCtx writer = {0};
    assert(crypt_init_encrypt(&writer, header, &key) == 0);
    assert(encrypt(&writer, &buffer, ad, sizeof(ad), true) == 0);
    size_t size = buffer.size;

    for (size_t i = 0; i < size + 3; i++) {
        uint8_t bytes[sizeof(saved)], changed_header[sizeof(header)], changed_ad[sizeof(ad)];
        memcpy(bytes, saved, size);
        memcpy(changed_header, header, sizeof(header));
        memcpy(changed_ad, ad, sizeof(ad));
        Key changed_key = key;
        if (i < size) bytes[i] ^= 1;
        else if (i == size) changed_header[0] ^= 1;
        else if (i == size + 1) changed_ad[0] ^= 1;
        else changed_key.b[0] ^= 1;

        CryptCtx reader = {0};
        buffer = (Buffer){bytes, sizeof(bytes), size};
        assert(crypt_init_decrypt(&reader, changed_header, &changed_key) == 0);
        assert(decrypt(&reader, &buffer, changed_ad, sizeof(changed_ad), true) == -1);
        assert(buffer.size == 0 && sodium_is_zero(bytes, size));
        assert(sodium_is_zero((const uint8_t*)&reader, sizeof(reader)));
    }

    // Even an authenticated record is rejected when its end-of-stream tag is unexpected.
    for (uint8_t tag = 0; tag <= crypto_secretstream_xchacha20poly1305_TAG_FINAL; tag++) {
        uint8_t bytes[32];
        crypto_secretstream_xchacha20poly1305_state stream;
        assert(crypto_secretstream_xchacha20poly1305_init_push(&stream, header, key.b) == 0);
        assert(crypto_secretstream_xchacha20poly1305_push(&stream, bytes, NULL,
            (const uint8_t*)"abc", 3, NULL, 0, tag) == 0);
        CryptCtx reader = {0};
        buffer = (Buffer){bytes, sizeof(bytes), 3 + CRYPT_RECORD_OVERHEAD};
        assert(crypt_init_decrypt(&reader, header, &key) == 0);
        if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL) {
            assert(decrypt(&reader, &buffer, NULL, 0, true) == 0);
        } else {
            assert(decrypt(&reader, &buffer, NULL, 0, true) == -1);
            assert(buffer.size == 0 && sodium_is_zero(bytes, 3 + CRYPT_RECORD_OVERHEAD));
        }
        sodium_memzero(&stream, sizeof(stream));
    }
}

static void multiple_records(void) {
    uint8_t header[CRYPT_HEADER_SIZE], first[32] = "first", last[32] = "last";
    CryptCtx writer = {0}, reader = {0};
    assert(crypt_init_encrypt(&writer, header, &key) == 0);
    Buffer a = {first, sizeof(first), 5}, b = {last, sizeof(last), 4};
    assert(encrypt(&writer, &a, ad, sizeof(ad), false) == 0);
    assert(writer.mode == CRYPT_ENCRYPTING);
    assert(encrypt(&writer, &b, ad, sizeof(ad), true) == 0);

    uint8_t saved_first[32], saved_last[32];
    memcpy(saved_first, first, a.size);
    memcpy(saved_last, last, b.size);
    assert(crypt_init_decrypt(&reader, header, &key) == 0);
    assert(decrypt(&reader, &a, ad, sizeof(ad), false) == 0);
    assert(reader.mode == CRYPT_DECRYPTING);
    assert(a.size == 5 && memcmp(first, "first", 5) == 0);
    assert(decrypt(&reader, &b, ad, sizeof(ad), true) == 0);
    assert(b.size == 4 && memcmp(last, "last", 4) == 0);

    // A final record cannot be moved ahead of the first record.
    assert(crypt_init_decrypt(&reader, header, &key) == 0);
    b = (Buffer){saved_last, sizeof(saved_last), 4 + CRYPT_RECORD_OVERHEAD};
    assert(decrypt(&reader, &b, ad, sizeof(ad), true) == -1);

    // Replaying the first record does not advance the stream successfully.
    assert(crypt_init_decrypt(&reader, header, &key) == 0);
    memcpy(first, saved_first, 5 + CRYPT_RECORD_OVERHEAD);
    a = (Buffer){first, sizeof(first), 5 + CRYPT_RECORD_OVERHEAD};
    assert(decrypt(&reader, &a, ad, sizeof(ad), false) == 0);
    a = (Buffer){saved_first, sizeof(saved_first), 5 + CRYPT_RECORD_OVERHEAD};
    assert(decrypt(&reader, &a, ad, sizeof(ad), false) == -1);
}

static void feed_record(void) {
    FeedPage page = {0}, parsed = {0};
    assert(feed_page_init(&page, 7) == FEED_OK);
    uint8_t post[POST_SIZE] = {0};
    uint32_t blobs = 1;
    memcpy(post + POST_BLOB_COUNT_OFFSET, &blobs, sizeof(blobs));
    assert(feed_page_append_posts(&page, post, sizeof(post)) == FEED_OK);

    uint8_t bytes[FEED_PAGE_HEADER_SIZE + sizeof(post) + CRYPT_RECORD_OVERHEAD];
    size_t size;
    assert(marshal_feed_page(bytes, sizeof(bytes), &size, &page) == FEED_OK);
    Buffer buffer = {bytes, sizeof(bytes), size};
    CryptCtx writer = {0}, reader = {0};
    uint8_t header[CRYPT_HEADER_SIZE];
    assert(crypt_init_encrypt(&writer, header, &key) == 0);
    assert(encrypt(&writer, &buffer, NULL, 0, true) == 0);
    assert(crypt_init_decrypt(&reader, header, &key) == 0);
    assert(decrypt(&reader, &buffer, NULL, 0, true) == 0);
    assert(parse_feed_page(&parsed, buffer.b, buffer.size) == FEED_OK);
    memset(bytes, 0, sizeof(bytes));
    assert(parsed.index == 7 && parsed.posts.size == 1);
    assert(memcmp(parsed.posts.data, post, sizeof(post)) == 0);

    feed_page_destroy(&page);
    feed_page_destroy(&parsed);
}

int main(void) {
    assert(sodium_init() >= 0);
    round_trip(0);
    round_trip(1);
    round_trip(132);
    round_trip(CRYPT_RECORD_MAX);
    capacity_and_arguments();
    failures();
    multiple_records();
    feed_record();
    return 0;
}
