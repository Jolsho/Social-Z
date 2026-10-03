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

typedef struct BlobRequest {
    Key owner;
    HashT label;
    HashT ciphertext_hash; // Used only by insertion.
    Signature signature;  // Used only by insertion and deletion.
} BlobRequest;

struct Request;

// BLAKE3(label || owner); this derives an address, not proof of authority.
void blob_locator(HashT* out, const HashT* label, const Key* owner);

// Mutation digest: label || kind:u16, plus ciphertext_hash for insertion.
// Get and unknown kinds are not signable. Failed calls leave outputs unchanged.
int blob_request_signing_hash(HashT* out, const struct Request* request);
int sign_blob_request(struct Request* request, const SigningKey* key);
// Check mutation signatures only. Unsigned retrieval returns false here.
bool valid_blob_request(const struct Request* request);

#ifdef __cplusplus
}
#endif
