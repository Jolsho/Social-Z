/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/owner.h"
#include "sz_common/crypto.h"
#include "sz_common/hash.h"
#include <assert.h>

static OwnerUpdate fixture(const KeyPair* keys) {
    OwnerUpdate update = {
        .owner = keys->pub, .identifier = OWNER_FEED_PAGE,
        .index = UINT64_C(0x0102030405060708), .new_revision = 1,
        .blob_size = UINT64_C(0x1122334455667788),
    };
    memset(update.blob_hash.b, 0x55, HASH_SIZE);
    return update;
}

static void wire_and_hashes(const KeyPair* keys) {
    OwnerUpdate update = fixture(keys), parsed;
    assert(sign_owner_update(&update, &keys->priv) == 0);
    assert(valid_owner_update(&update));
    uint8_t expected[166] = {
        [1] = 1, [3] = 1, [37] = 2,
        [38] = 1, [39] = 2, [40] = 3, [41] = 4,
        [42] = 5, [43] = 6, [44] = 7, [45] = 8, [61] = 1,
        [94] = 0x11, [95] = 0x22, [96] = 0x33, [97] = 0x44,
        [98] = 0x55, [99] = 0x66, [100] = 0x77, [101] = 0x88,
    };
    memcpy(expected + 4, keys->pub.b, KEY_SIZE);
    memset(expected + 62, 0x55, HASH_SIZE);
    memcpy(expected + 102, update.signature.b, SIGNATURE_SIZE);
    uint8_t bytes[OWNER_UPDATE_SIZE];
    size_t size = 0;
    assert(marshal_owner_update(bytes, sizeof(bytes), &size, &update) == 0);
    assert(size == sizeof(expected) && memcmp(bytes, expected, size) == 0);
    assert(parse_owner_update(&parsed, bytes, size) == 0 && valid_owner_update(&parsed));
    assert(marshal_owner_update(bytes, sizeof(bytes), &size, &parsed) == 0);
    assert(memcmp(bytes, expected, size) == 0);

    // Independently assemble the documented signature and locator preimages.
    Hasher hasher = new_hasher();
    hash_update(&hasher, (const uint8_t*)"sz.record.v1", 12);
    hash_update(&hasher, expected, 102);
    HashT wanted = hash_finalize(&hasher), actual;
    assert(owner_update_signing_hash(&actual, &update) == 0);
    assert(hash_is_equal(&actual, &wanted));
    uint8_t locator[42] = {0, 2, 1, 2, 3, 4, 5, 6, 7, 8};
    memcpy(locator + 10, keys->pub.b, KEY_SIZE);
    hasher = new_hasher();
    hash_update(&hasher, (const uint8_t*)"sz.locator.v1", 13);
    hash_update(&hasher, locator, sizeof(locator));
    wanted = hash_finalize(&hasher);
    assert(owner_update_locator(&actual, &update) == 0 && hash_is_equal(&actual, &wanted));
    update.new_revision = 2;
    update.expected_revision = 1;
    update.blob_size++;
    update.blob_hash.b[0]++;
    assert(owner_update_locator(&actual, &update) == 0 && hash_is_equal(&actual, &wanted));
    update.index++;
    assert(owner_update_locator(&actual, &update) == 0 && !hash_is_equal(&actual, &wanted));
    update.index--;
    update.identifier = OWNER_ACCOUNT;
    assert(owner_update_locator(&actual, &update) == 0 && !hash_is_equal(&actual, &wanted));
    update.identifier = OWNER_FEED_PAGE;
    update.owner.b[0] ^= 1;
    assert(owner_update_locator(&actual, &update) == 0 && !hash_is_equal(&actual, &wanted));
}

static void rejected(OwnerUpdate* out, const uint8_t* bytes, size_t size) {
    memset(out, 0xa5, sizeof(*out));
    OwnerUpdate before = *out;
    assert(parse_owner_update(out, bytes, size) == -1);
    assert(memcmp(out, &before, sizeof(*out)) == 0);
}

static void malformed_and_tampered(const KeyPair* keys) {
    OwnerUpdate update = fixture(keys), parsed;
    assert(sign_owner_update(&update, &keys->priv) == 0);
    uint8_t bytes[OWNER_UPDATE_SIZE + 1], changed[sizeof(bytes)];
    size_t size;
    assert(marshal_owner_update(bytes, sizeof(bytes), &size, &update) == 0);
    for (size_t n = 0; n < size; n++) rejected(&parsed, bytes, n);
    bytes[size] = 0;
    rejected(&parsed, bytes, size + 1);
    rejected(&parsed, bytes, sizeof(bytes));
    rejected(&parsed, NULL, size);
    assert(parse_owner_update(NULL, bytes, size) == -1);
    // Every encoded byte is either structurally checked or covered by the signature.
    for (size_t i = 0; i < size; i++) {
        memcpy(changed, bytes, size);
        changed[i] ^= 1;
        if (parse_owner_update(&parsed, changed, size) == 0) assert(!valid_owner_update(&parsed));
    }
    memcpy(changed, bytes, size);
    changed[53] = 1; changed[61] = 2; // Valid revision increment, invalid signature.
    assert(parse_owner_update(&parsed, changed, size) == 0 && !valid_owner_update(&parsed));
    const uint8_t identifiers[] = {0, 4, 255};
    for (size_t i = 0; i < sizeof(identifiers); i++) {
        memcpy(changed, bytes, size);
        changed[37] = identifiers[i];
        rejected(&parsed, changed, size);
    }
    memcpy(changed, bytes, size);
    changed[36] = 1;
    rejected(&parsed, changed, size);
    memset(changed, 0xa5, sizeof(changed));
    size_t unchanged_size = 999;
    assert(marshal_owner_update(changed, size - 1, &unchanged_size, &update) == -1);
    assert(unchanged_size == 999);
    for (size_t i = 0; i < sizeof(changed); i++) assert(changed[i] == 0xa5);
    KeyPair other;
    assert(new_keypair(&other) == 0);
    Signature before = update.signature;
    assert(sign_owner_update(&update, &other.priv) == -1);
    assert(memcmp(&before, &update.signature, sizeof(before)) == 0);
    assert(!valid_owner_update(NULL));
}

static void bounds_and_identifiers(const KeyPair* keys) {
    OwnerUpdate update = fixture(keys), parsed;
    uint8_t bytes[OWNER_UPDATE_SIZE];
    size_t size;
    update.expected_revision = UINT64_MAX - 1;
    update.new_revision = update.index = update.blob_size = UINT64_MAX;
    assert(sign_owner_update(&update, &keys->priv) == 0);
    assert(marshal_owner_update(bytes, sizeof(bytes), &size, &update) == 0);
    assert(size == OWNER_UPDATE_SIZE);
    assert(parse_owner_update(&parsed, bytes, size) == 0 && valid_owner_update(&parsed));
    assert(parsed.index == UINT64_MAX && parsed.blob_size == UINT64_MAX && parsed.new_revision == UINT64_MAX);
    update.expected_revision = UINT64_MAX;
    update.new_revision = 0;
    assert(marshal_owner_update(bytes, sizeof(bytes), &size, &update) == -1);
    update.expected_revision = 0;
    update.new_revision = 0;
    assert(marshal_owner_update(bytes, sizeof(bytes), &size, &update) == -1);
    update.new_revision = 2;
    assert(marshal_owner_update(bytes, sizeof(bytes), &size, &update) == -1);
    update.new_revision = 1;
    update.blob_size = 0;
    assert(marshal_owner_update(bytes, sizeof(bytes), &size, &update) == -1);
    update.blob_size = 1;
    const OwnerIdentifier invalid[] = {0, 4, (OwnerIdentifier)-1, (OwnerIdentifier)65536};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        update.identifier = invalid[i];
        assert(marshal_owner_update(bytes, sizeof(bytes), &size, &update) == -1);
        HashT locator;
        assert(owner_update_locator(&locator, &update) == -1);
    }
    for (OwnerIdentifier id = OWNER_ACCOUNT; id <= OWNER_RECIPIENT_PAGE; id++) {
        update.identifier = id;
        assert(sign_owner_update(&update, &keys->priv) == 0);
        assert(marshal_owner_update(bytes, sizeof(bytes), &size, &update) == 0);
        assert(parse_owner_update(&parsed, bytes, size) == 0 && valid_owner_update(&parsed));
        assert(parsed.identifier == id);
    }
}

int main(void) {
    KeyPair keys;
    assert(new_keypair(&keys) == 0);
    wire_and_hashes(&keys);
    malformed_and_tampered(&keys);
    bounds_and_identifiers(&keys);
    return 0;
}
