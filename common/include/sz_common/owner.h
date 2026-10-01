/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_common/codec.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum OwnerIdentifier {
    OWNER_ACCOUNT = 1,
    OWNER_FEED_PAGE = 2,
    OWNER_RECIPIENT_PAGE = 3,
} OwnerIdentifier;

#define OWNER_UPDATE_SIZE 166

typedef struct OwnerUpdate {
    Key owner;
    OwnerIdentifier identifier;
    uint64_t index, expected_revision, new_revision;
    HashT blob_hash;
    uint64_t blob_size;
    Signature signature;
} OwnerUpdate;

/* V1, kind 1: owner, identifier:u16, index, expected/new revision,
 * blob hash, blob size, signature. Integers are big-endian; native enum representation is never serialized.
 * Return 0 or -1. Failed calls leave outputs unchanged.
 * Parsing checks structure; signature and storage authority are separate checks.
 */
int marshal_owner_update(uint8_t* out, size_t capacity, size_t* size, const OwnerUpdate* update);
int parse_owner_update(OwnerUpdate* out, const uint8_t* bytes, size_t size);
int owner_update_signing_hash(HashT* out, const OwnerUpdate* update);
int owner_update_locator(HashT* out, const OwnerUpdate* update);
int sign_owner_update(OwnerUpdate* update, const SigningKey* key);
bool valid_owner_update(const OwnerUpdate* update);

#ifdef __cplusplus
}
#endif
