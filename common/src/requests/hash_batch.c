/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/requests/requests.h"
#include "sz_common/hash.h"
#include "sz_common/crypto.h"

static void write_uint(uint8_t* out, uint64_t value, size_t size) {
    while (size) {
        out[--size] = (uint8_t)value;
        value >>= 8;
    }
}

int hash_batch_signing_hash(HashT* out, const Request* request) {
    if (!out || !request ||
        (request->kind != REQUEST_NEW_PERMISSIONS &&
         request->kind != REQUEST_PUBLISH_VOUCHERS &&
         request->kind != REQUEST_REVOKE_PERMISSIONS)) {
        return -1;
    }

    const HashBatch* batch = &request->data.hash_batch;
    uint8_t header[HASH_BATCH_HEADER_SIZE + 8];
    write_uint(header, 1, 2);
    write_uint(header + 2, request->kind, 2);
    memcpy(header + 4, batch->owner.b, KEY_SIZE);
    write_uint(header + 36, batch->count, 4);

    size_t header_size = HASH_BATCH_HEADER_SIZE;
    if (request->kind == REQUEST_PUBLISH_VOUCHERS) {
        write_uint(header + header_size, batch->expires, 8);
        header_size += 8;
    }

    // Hash the same bytes the marshaler writes, without allocating a complete batch.
    Hasher hasher = new_hasher();
    hash_update(&hasher, header, header_size);
    for (uint32_t i = 0; i < batch->count; i++) {
        hash_update(&hasher, batch->hashes[i].b, HASH_SIZE);
    }
    *out = hash_finalize(&hasher);
    return 0;
}

int sign_hash_batch_request(Request* request, const SigningKey* key) {
    HashT hash;
    Signature signature;
    if (!key || hash_batch_signing_hash(&hash, request) != 0 ||
        sign_hash(key, &signature, &hash) != 0) {
        return -1;
    }

    Key owner = request->data.hash_batch.owner;
    if (!valid_signature(&owner, &signature, &hash)) {
        return -1;
    }

    request->data.hash_batch.signature = signature;
    return 0;
}

bool valid_hash_batch_request(const Request* request) {
    HashT hash;
    if (hash_batch_signing_hash(&hash, request) != 0) {
        return false;
    }

    Key owner = request->data.hash_batch.owner;
    Signature signature = request->data.hash_batch.signature;
    return valid_signature(&owner, &signature, &hash);
}
