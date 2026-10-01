/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/owner.h"
#include "sz_common/crypto.h"
#include "sz_common/hash.h"

static uint64_t read_uint(const uint8_t* p, size_t n) {
    uint64_t value = 0;

    for (size_t i = 0; i < n; i++)
        value = (value << 8) | p[i];

    return value;
}

static void write_uint(
    uint8_t* p,
    uint64_t value,
    size_t n
) {
    while (n) {
        p[--n] = (uint8_t)value;
        value >>= 8;
    }
}

enum { UNSIGNED_SIZE = OWNER_UPDATE_SIZE - SIGNATURE_SIZE };

static bool valid_identifier(OwnerIdentifier identifier) {
    return identifier >= OWNER_ACCOUNT && identifier <= OWNER_RECIPIENT_PAGE;
}

static bool valid_fields(const OwnerUpdate* update) {
    return update && valid_identifier(update->identifier) && update->blob_size &&
        update->expected_revision != UINT64_MAX &&
        update->new_revision == update->expected_revision + 1;
}

static void encode_unsigned(uint8_t* out, const OwnerUpdate* update) {
    write_uint(out, 1, 2);
    write_uint(out + 2, 1, 2);

    memcpy(out + 4, update->owner.b, KEY_SIZE);
    write_uint(out + 36, update->identifier, 2);

    uint8_t* p = out + 38;
    write_uint(p, update->index, 8);
    write_uint(p + 8, update->expected_revision, 8);
    write_uint(p + 16, update->new_revision, 8);
    memcpy(p + 24, update->blob_hash.b, HASH_SIZE);

    write_uint(p + 56, update->blob_size, 8);
}

int marshal_owner_update(
    uint8_t* out,
    size_t capacity,
    size_t* size,
    const OwnerUpdate* update
) {
    if (!out || !size || !valid_fields(update) || capacity < OWNER_UPDATE_SIZE)
        return -1;

    encode_unsigned(out, update);

    memcpy(out + UNSIGNED_SIZE, update->signature.b, SIGNATURE_SIZE);

    *size = OWNER_UPDATE_SIZE;

    return 0;
}

int parse_owner_update(
    OwnerUpdate* out,
    const uint8_t* bytes,
    size_t size
) {
    if (!out || !bytes || size != OWNER_UPDATE_SIZE ||
        read_uint(bytes, 2) != 1 || read_uint(bytes + 2, 2) != 1)
        return -1;

    OwnerUpdate update = {0};
    memcpy(update.owner.b, bytes + 4, KEY_SIZE);
    update.identifier = (OwnerIdentifier)read_uint(bytes + 36, 2);

    const uint8_t* p = bytes + 38;
    update.index = read_uint(p, 8);
    update.expected_revision = read_uint(p + 8, 8);
    update.new_revision = read_uint(p + 16, 8);
    memcpy(update.blob_hash.b, p + 24, HASH_SIZE);
    update.blob_size = read_uint(p + 56, 8);
    memcpy(update.signature.b, p + 64, SIGNATURE_SIZE);

    if (!valid_fields(&update))
        return -1;

    *out = update;

    return 0;
}

int owner_update_signing_hash(HashT* out, const OwnerUpdate* update) {
    if (!out || !valid_fields(update))
        return -1;

    uint8_t bytes[UNSIGNED_SIZE];
    encode_unsigned(bytes, update);

    static const uint8_t prefix[] = "sz.record.v1";
    Hasher hasher = new_hasher();
    hash_update(&hasher, prefix, sizeof(prefix) - 1);
    hash_update(&hasher, bytes, sizeof(bytes));
    *out = hash_finalize(&hasher);

    return 0;
}

int owner_update_locator(HashT* out, const OwnerUpdate* update) {
    if (!out || !update || !valid_identifier(update->identifier))
        return -1;

    uint8_t bytes[2 + 8 + KEY_SIZE];
    write_uint(bytes, update->identifier, 2);
    write_uint(bytes + 2, update->index, 8);
    memcpy(bytes + 10, update->owner.b, KEY_SIZE);

    static const uint8_t prefix[] = "sz.locator.v1";
    Hasher hasher = new_hasher();
    hash_update(&hasher, prefix, sizeof(prefix) - 1);
    hash_update(&hasher, bytes, sizeof(bytes));
    *out = hash_finalize(&hasher);

    return 0;
}

int sign_owner_update(OwnerUpdate* update, const SigningKey* key) {
    HashT hash;
    Signature signature;

    if (!key || owner_update_signing_hash(&hash, update) != 0 ||
        sign_hash(key, &signature, &hash) != 0 ||
        !valid_signature(&update->owner, &signature, &hash))
        return -1;

    update->signature = signature;

    return 0;
}

bool valid_owner_update(const OwnerUpdate* update) {
    HashT hash;
    if (owner_update_signing_hash(&hash, update) != 0)
        return false;

    Key owner = update->owner;
    Signature signature = update->signature;

    return valid_signature(&owner, &signature, &hash);
}
