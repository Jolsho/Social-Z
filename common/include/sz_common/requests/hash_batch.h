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

typedef struct HashBatch {
    Key owner;
    HashT* hashes;
    uint32_t count;
    uint32_t capacity; // Caller-owned hash storage available to the parser.
    uint64_t expires;  // UTC seconds for the whole voucher batch; unused for permissions.
    Signature signature;
} HashBatch;

struct Request;

// Sign the canonical request prefix, owner, count, expiry when present, and hashes.
// Only permission registration/revocation and voucher publication are signable here.
// Failed calls leave outputs unchanged; hash storage must remain valid during the call.
int hash_batch_signing_hash(HashT* out, const struct Request* request);
int sign_hash_batch_request(struct Request* request, const SigningKey* key);
bool valid_hash_batch_request(const struct Request* request);

#ifdef __cplusplus
}
#endif
