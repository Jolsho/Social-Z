/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/requests/requests.h"
#include "sz_common/hash.h"
#include "sz_common/crypto.h"

void blob_locator(HashT* out, const HashT* label, const Key* owner) {
    Hasher hasher = new_hasher();
    hash_update(&hasher, label->b, HASH_SIZE);
    hash_update(&hasher, owner->b, KEY_SIZE);
    *out = hash_finalize(&hasher);
}

int blob_request_signing_hash(HashT* out, const Request* request) {
    if (!out || !request ||
        (request->kind != REQUEST_BLOB_PUT && request->kind != REQUEST_BLOB_DELETE)) {
        return -1;
    }

    // The operation has the same two-byte encoding as the request prefix.
    uint8_t operation[] = {(uint8_t)(request->kind >> 8), (uint8_t)request->kind};
    const BlobRequest* blob = &request->data.blob;
    Hasher hasher = new_hasher();
    hash_update(&hasher, blob->label.b, HASH_SIZE);
    hash_update(&hasher, operation, sizeof(operation));
    if (request->kind == REQUEST_BLOB_PUT) {
        hash_update(&hasher, blob->ciphertext_hash.b, HASH_SIZE);
    }
    *out = hash_finalize(&hasher);
    return 0;
}

int sign_blob_request(Request* request, const SigningKey* key) {
    HashT hash;
    Signature signature;
    if (!key || blob_request_signing_hash(&hash, request) != 0 ||
        sign_hash(key, &signature, &hash) != 0) {
        return -1;
    }

    Key owner = request->data.blob.owner;
    if (!valid_signature(&owner, &signature, &hash)) {
        return -1;
    }

    request->data.blob.signature = signature;
    return 0;
}

bool valid_blob_request(const Request* request) {
    HashT hash;
    if (blob_request_signing_hash(&hash, request) != 0) {
        return false;
    }

    Key owner = request->data.blob.owner;
    Signature signature = request->data.blob.signature;
    return valid_signature(&owner, &signature, &hash);
}
