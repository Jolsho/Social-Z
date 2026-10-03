/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/requests/requests.h"
#include "sz_common/hash.h"
#include <sodium.h>
#include <assert.h>

static HashT hash_bytes(const uint8_t* bytes, size_t size) {
    blake3_hasher hasher;
    HashT hash;
    blake3_hasher_init(&hasher);
    blake3_hasher_update(&hasher, bytes, size);
    blake3_hasher_finalize(&hasher, hash.b, HASH_SIZE);
    return hash;
}

static void hashes_and_authority(Request* request, const KeyPair* keys) {
    const BlobRequest* blob = &request->data.blob;
    uint8_t locator_bytes[64];
    memcpy(locator_bytes, blob->label.b, 32);
    memcpy(locator_bytes + 32, keys->pub.b, 32);
    HashT expected = hash_bytes(locator_bytes, sizeof(locator_bytes));
    HashT actual;
    blob_locator(&actual, &blob->label, &blob->owner);
    assert(hash_is_equal(&actual, &expected));

    Key other_owner = keys->pub;
    other_owner.b[0] ^= 1;
    blob_locator(&actual, &blob->label, &other_owner);
    assert(!hash_is_equal(&actual, &expected));

    uint8_t signed_bytes[66];
    memcpy(signed_bytes, blob->label.b, 32);
    signed_bytes[32] = 0;
    signed_bytes[33] = REQUEST_BLOB_PUT;
    memcpy(signed_bytes + 34, blob->ciphertext_hash.b, 32);
    expected = hash_bytes(signed_bytes, sizeof(signed_bytes));
    assert(blob_request_signing_hash(&actual, request) == 0);
    assert(hash_is_equal(&actual, &expected));
    assert(sign_blob_request(request, &keys->priv) == 0);
    assert(valid_blob_request(request));

    Request changed = *request;
    changed.data.blob.label.b[0] ^= 1;
    assert(!valid_blob_request(&changed));
    changed = *request;
    changed.data.blob.owner.b[0] ^= 1;
    assert(!valid_blob_request(&changed));
    changed = *request;
    changed.data.blob.ciphertext_hash.b[0] ^= 1;
    assert(!valid_blob_request(&changed));
    changed = *request;
    changed.data.blob.signature.b[0] ^= 1;
    assert(!valid_blob_request(&changed));
    changed = *request;
    changed.kind = REQUEST_BLOB_DELETE;
    assert(!valid_blob_request(&changed));

    signed_bytes[33] = REQUEST_BLOB_DELETE;
    expected = hash_bytes(signed_bytes, 34);
    assert(blob_request_signing_hash(&actual, &changed) == 0);
    assert(hash_is_equal(&actual, &expected));
    assert(sign_blob_request(&changed, &keys->priv) == 0);
    assert(valid_blob_request(&changed));
    changed.kind = REQUEST_BLOB_PUT;
    assert(!valid_blob_request(&changed));

    KeyPair wrong_keys;
    uint8_t seed[32] = {9};
    assert(crypto_sign_seed_keypair(wrong_keys.pub.b, wrong_keys.priv.b, seed) == 0);
    Signature before = request->data.blob.signature;
    assert(sign_blob_request(request, &wrong_keys.priv) == -1);
    assert(memcmp(before.b, request->data.blob.signature.b, SIGNATURE_SIZE) == 0);
    sodium_memzero(&wrong_keys, sizeof(wrong_keys));

    changed.kind = REQUEST_BLOB_GET;
    assert(sign_blob_request(&changed, &keys->priv) == -1);
    assert(!valid_blob_request(&changed));
    actual = expected;
    assert(blob_request_signing_hash(&actual, &changed) == -1);
    assert(hash_is_equal(&actual, &expected));
    changed.kind = (RequestKind)0xffff;
    assert(sign_blob_request(&changed, &keys->priv) == -1);
    assert(!valid_blob_request(&changed));
}

static void request_bytes(const Request* insertion, const KeyPair* keys) {
    const RequestKind kinds[] = {REQUEST_BLOB_GET, REQUEST_BLOB_PUT, REQUEST_BLOB_DELETE};
    const size_t sizes[] = {68, 164, 132};
    uint8_t bytes[BLOB_PUT_REQUEST_SIZE + 1];

    for (size_t i = 0; i < sizeof(kinds) / sizeof(kinds[0]); i++) {
        Request request = *insertion;
        request.kind = kinds[i];
        if (request.kind != REQUEST_BLOB_GET) {
            assert(sign_blob_request(&request, &keys->priv) == 0);
        }
        assert(marshal_blob_request(bytes, &request) == sizes[i]);
        assert(bytes[0] == 0 && bytes[1] == 1 && bytes[2] == 0 && bytes[3] == kinds[i]);
        assert(memcmp(bytes + 4, request.data.blob.owner.b, 32) == 0);
        assert(memcmp(bytes + 36, request.data.blob.label.b, 32) == 0);
        if (request.kind == REQUEST_BLOB_PUT) {
            assert(memcmp(bytes + 68, request.data.blob.ciphertext_hash.b, 32) == 0);
            assert(memcmp(bytes + 100, request.data.blob.signature.b, 64) == 0);
        } else if (request.kind == REQUEST_BLOB_DELETE) {
            assert(memcmp(bytes + 68, request.data.blob.signature.b, 64) == 0);
        }

        Request parsed;
        memset(&parsed, 0xaa, sizeof(parsed));
        assert(parse_request(&parsed, bytes, sizes[i]) == 0);
        assert(parsed.kind == request.kind);
        assert(hash_is_equal(&parsed.data.blob.label, &request.data.blob.label));
        assert(memcmp(parsed.data.blob.owner.b, keys->pub.b, KEY_SIZE) == 0);
        if (request.kind != REQUEST_BLOB_PUT) {
            assert(sodium_is_zero(parsed.data.blob.ciphertext_hash.b, HASH_SIZE));
        }
        if (request.kind == REQUEST_BLOB_GET) {
            assert(sodium_is_zero(parsed.data.blob.signature.b, SIGNATURE_SIZE));
        } else {
            assert(valid_blob_request(&parsed));
        }

        uint8_t before[sizeof(parsed)];
        memcpy(before, &parsed, sizeof(parsed));
        for (size_t size = 0; size < sizes[i]; size++) {
            assert(parse_request(&parsed, bytes, size) == -1);
            assert(memcmp(before, &parsed, sizeof(parsed)) == 0);
        }
        assert(parse_request(&parsed, bytes, sizes[i] + 1) == -1);
        bytes[1] = 2;
        assert(parse_request(&parsed, bytes, sizes[i]) == -1);
        bytes[1] = 1;
        bytes[2] = bytes[3] = 0xff;
        assert(parse_request(&parsed, bytes, sizes[i]) == -1);

        // Neither parsed fields nor authentication depend on the host's input lifetime.
        memset(bytes, 0, sizeof(bytes));
        assert(memcmp(before, &parsed, sizeof(parsed)) == 0);
        if (request.kind != REQUEST_BLOB_GET) {
            assert(valid_blob_request(&parsed));
        }
    }

    Request invalid = {.kind = REQUEST_ACCOUNT};
    bytes[0] = 0xaa;
    assert(marshal_blob_request(bytes, &invalid) == 0);
    assert(bytes[0] == 0xaa);
}

int main(void) {
    assert(sodium_init() >= 0);
    KeyPair keys;
    uint8_t seed[32] = {1, 2, 3};
    assert(crypto_sign_seed_keypair(keys.pub.b, keys.priv.b, seed) == 0);
    Request request = {.kind = REQUEST_BLOB_PUT};
    request.data.blob.owner = keys.pub;
    memset(request.data.blob.label.b, 0xaa, HASH_SIZE);
    memset(request.data.blob.ciphertext_hash.b, 0xbb, HASH_SIZE);

    hashes_and_authority(&request, &keys);
    request_bytes(&request, &keys);
    sodium_memzero(&keys, sizeof(keys));
    return 0;
}
