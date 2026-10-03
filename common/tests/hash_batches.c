/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/requests/requests.h"
#include "sz_common/hash.h"
#include <sodium.h>
#include <assert.h>

static void check_batch(RequestKind kind, const KeyPair* keys) {
    HashT hashes[2];
    memset(hashes[0].b, 0xaa, HASH_SIZE);
    memset(hashes[1].b, 0xbb, HASH_SIZE);
    Request request = {.kind = kind};
    request.data.hash_batch = (HashBatch){
        .owner = keys->pub, .hashes = hashes, .count = 2,
        .expires = UINT64_C(0x0102030405060708),
    };
    assert(sign_hash_batch_request(&request, &keys->priv) == 0);
    assert(valid_hash_batch_request(&request));

    uint8_t bytes[HASH_BATCH_REQUEST_BASE_SIZE + 8 + sizeof(hashes) + 1];
    size_t expiry_size = kind == REQUEST_PUBLISH_VOUCHERS ? 8 : 0;
    size_t size = marshal_hash_batch_request(bytes, &request);
    assert(size == HASH_BATCH_REQUEST_BASE_SIZE + expiry_size + sizeof(hashes));
    assert(bytes[0] == 0 && bytes[1] == 1 && bytes[2] == 0 && bytes[3] == kind);
    assert(memcmp(bytes + 4, keys->pub.b, KEY_SIZE) == 0);
    assert(bytes[36] == 0 && bytes[37] == 0 && bytes[38] == 0 && bytes[39] == 2);
    if (expiry_size) {
        for (size_t i = 0; i < expiry_size; i++) {
            assert(bytes[40 + i] == i + 1);
        }
    }
    assert(memcmp(bytes + 40 + expiry_size, hashes, sizeof(hashes)) == 0);
    assert(memcmp(bytes + size - SIGNATURE_SIZE,
        request.data.hash_batch.signature.b, SIGNATURE_SIZE) == 0);

    // Authentication covers the exact wire bytes before the signature.
    Hasher hasher = new_hasher();
    hash_update(&hasher, bytes, size - SIGNATURE_SIZE);
    HashT expected = hash_finalize(&hasher);
    HashT actual;
    assert(hash_batch_signing_hash(&actual, &request) == 0);
    assert(hash_is_equal(&actual, &expected));

    HashT storage[2];
    Request parsed = {0};
    parsed.data.hash_batch.hashes = storage;
    parsed.data.hash_batch.capacity = 2;
    assert(parse_request(&parsed, bytes, size) == 0);
    assert(parsed.kind == kind && parsed.data.hash_batch.count == 2);
    assert(parsed.data.hash_batch.expires == (expiry_size ? request.data.hash_batch.expires : 0));
    assert(valid_hash_batch_request(&parsed));

    Request before = parsed;
    for (size_t truncated = 0; truncated < size; truncated++) {
        assert(parse_request(&parsed, bytes, truncated) == -1);
        assert(memcmp(&parsed, &before, sizeof(parsed)) == 0);
        assert(memcmp(storage, hashes, sizeof(hashes)) == 0);
    }
    assert(parse_request(&parsed, bytes, size + 1) == -1);
    bytes[39] = 3;
    assert(parse_request(&parsed, bytes, size) == -1);
    bytes[39] = 2;
    parsed.data.hash_batch.capacity = 1;
    assert(parse_request(&parsed, bytes, size) == -1);
    parsed.data.hash_batch.capacity = 2;
    parsed.data.hash_batch.hashes = NULL;
    assert(parse_request(&parsed, bytes, size) == -1);
    parsed = before;

    // Parsed hashes outlive the borrowed networking buffer.
    memset(bytes, 0, sizeof(bytes));
    assert(valid_hash_batch_request(&parsed));
    storage[0].b[0] ^= 1;
    assert(!valid_hash_batch_request(&parsed));
    storage[0] = hashes[0];
    parsed.data.hash_batch.owner.b[0] ^= 1;
    assert(!valid_hash_batch_request(&parsed));
    parsed = before;
    parsed.data.hash_batch.signature.b[0] ^= 1;
    assert(!valid_hash_batch_request(&parsed));
    parsed = before;
    parsed.kind = kind == REQUEST_REVOKE_PERMISSIONS
        ? REQUEST_NEW_PERMISSIONS : REQUEST_REVOKE_PERMISSIONS;
    assert(!valid_hash_batch_request(&parsed));
    parsed = before;
    if (expiry_size) {
        parsed.data.hash_batch.expires++;
        assert(!valid_hash_batch_request(&parsed));
    }

    // Empty batches need no hash storage, but are still signed.
    request.data.hash_batch.count = 0;
    request.data.hash_batch.hashes = NULL;
    assert(sign_hash_batch_request(&request, &keys->priv) == 0);
    size = marshal_hash_batch_request(bytes, &request);
    parsed = (Request){0};
    assert(parse_request(&parsed, bytes, size) == 0);
    assert(parsed.data.hash_batch.count == 0 && valid_hash_batch_request(&parsed));
}

int main(void) {
    assert(sodium_init() >= 0);
    KeyPair keys, other;
    uint8_t seed[32] = {1};
    assert(crypto_sign_seed_keypair(keys.pub.b, keys.priv.b, seed) == 0);
    seed[0] = 2;
    assert(crypto_sign_seed_keypair(other.pub.b, other.priv.b, seed) == 0);

    check_batch(REQUEST_NEW_PERMISSIONS, &keys);
    check_batch(REQUEST_REVOKE_PERMISSIONS, &keys);
    check_batch(REQUEST_PUBLISH_VOUCHERS, &keys);

    Request request = {.kind = REQUEST_NEW_PERMISSIONS};
    request.data.hash_batch.owner = keys.pub;
    memset(request.data.hash_batch.signature.b, 0xaa, SIGNATURE_SIZE);
    Signature before = request.data.hash_batch.signature;
    assert(sign_hash_batch_request(&request, &other.priv) == -1);
    assert(memcmp(before.b, request.data.hash_batch.signature.b, SIGNATURE_SIZE) == 0);

    request.kind = REQUEST_BLOB_GET;
    uint8_t bytes[1] = {0xaa};
    assert(marshal_hash_batch_request(bytes, &request) == 0 && bytes[0] == 0xaa);
    assert(sign_hash_batch_request(&request, &keys.priv) == -1);
    assert(!valid_hash_batch_request(&request));
    sodium_memzero(&keys, sizeof(keys));
    sodium_memzero(&other, sizeof(other));
    return 0;
}
