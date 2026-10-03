/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/requests/requests.h"

static uint64_t read_uint(const uint8_t* p, size_t n) {
    uint64_t v = 0;

    for (size_t i = 0; i < n; i++) {
        v = (v << 8) | p[i];
    }

    return v;
}

static void write_uint(
    uint8_t* p,
    uint64_t v,
    size_t n
) {
    while (n) {
        p[--n] = (uint8_t)v;
        v >>= 8;
    }
}

size_t account_username_size(const char* username) {
    if (!username) {
        return 0;
    }

    size_t n = 0;
    while (n <= ACCOUNT_USERNAME_MAX && username[n]) {
        n++;
    }

    return n && n <= ACCOUNT_USERNAME_MAX ? n : 0;
}

size_t marshal_account_request(
    uint8_t out[6 + ACCOUNT_USERNAME_MAX],
    const char* username
) {
    size_t n = account_username_size(username);
    if (!out || !n) {
        return 0;
    }

    write_uint(out, 1, 2);
    write_uint(out + 2, REQUEST_ACCOUNT, 2);
    write_uint(out + 4, n, 2);
    memcpy(out + 6, username, n);

    return 6 + n;
}

size_t marshal_blob_request(uint8_t* out, const Request* request) {
    size_t size;
    switch (request->kind) {
    case REQUEST_BLOB_GET:
        size = BLOB_GET_REQUEST_SIZE;
        break;
    case REQUEST_BLOB_PUT:
        size = BLOB_PUT_REQUEST_SIZE;
        break;
    case REQUEST_BLOB_DELETE:
        size = BLOB_DELETE_REQUEST_SIZE;
        break;
    default:
        return 0;
    }

    const BlobRequest* blob = &request->data.blob;
    write_uint(out, 1, 2);
    write_uint(out + 2, request->kind, 2);
    memcpy(out + 4, blob->owner.b, KEY_SIZE);
    memcpy(out + 36, blob->label.b, HASH_SIZE);

    uint8_t* tail = out + BLOB_GET_REQUEST_SIZE;
    if (request->kind == REQUEST_BLOB_PUT) {
        memcpy(tail, blob->ciphertext_hash.b, HASH_SIZE);
        tail += HASH_SIZE;
    }
    if (request->kind != REQUEST_BLOB_GET) {
        memcpy(tail, blob->signature.b, SIGNATURE_SIZE);
    }

    return size;
}

int parse_request(Request* out, const uint8_t* bytes, size_t size) {
    if (!out || !bytes || size < REQUEST_HEADER_SIZE || read_uint(bytes, 2) != 1) {
        return -1;
    }

    RequestKind kind = (RequestKind)read_uint(bytes + 2, 2);
    switch (kind) {
    case REQUEST_ACCOUNT: {
        if (size < 6) {
            return -1;
        }

        size_t n = read_uint(bytes + 4, 2);
        if (!n || n > ACCOUNT_USERNAME_MAX || size != 6 + n ||
            memchr(bytes + 6, 0, n)) {
            return -1;
        }

        memcpy(out->data.username, bytes + 6, n);
        out->data.username[n] = 0;
        break;
    }
    case REQUEST_BLOB_GET:
    case REQUEST_BLOB_PUT:
    case REQUEST_BLOB_DELETE: {
        size_t expected = kind == REQUEST_BLOB_GET ? BLOB_GET_REQUEST_SIZE
            : kind == REQUEST_BLOB_PUT ? BLOB_PUT_REQUEST_SIZE : BLOB_DELETE_REQUEST_SIZE;
        if (size != expected) {
            return -1;
        }

        // Clear fields absent from GET/DELETE so previous request data cannot leak through.
        BlobRequest blob = {0};
        memcpy(blob.owner.b, bytes + 4, KEY_SIZE);
        memcpy(blob.label.b, bytes + 36, HASH_SIZE);

        const uint8_t* tail = bytes + BLOB_GET_REQUEST_SIZE;
        if (kind == REQUEST_BLOB_PUT) {
            memcpy(blob.ciphertext_hash.b, tail, HASH_SIZE);
            tail += HASH_SIZE;
        }
        if (kind != REQUEST_BLOB_GET) {
            memcpy(blob.signature.b, tail, SIGNATURE_SIZE);
        }

        out->data.blob = blob;
        break;
    }
    default:
        return -1;
    }

    out->kind = kind;
    return 0;
}

void marshal_account_response_metadata(
    uint8_t out[ACCOUNT_RESPONSE_METADATA_SIZE],
    const AccountResponseMetadata* metadata
) {
    write_uint(out, 1, 2);
    write_uint(out + 2, REQUEST_ACCOUNT, 2);
    memcpy(out + 4, metadata->account.b, KEY_SIZE);
    memcpy(out + 36, metadata->hash.b, HASH_SIZE);
    write_uint(out + 68, metadata->size, 4);
}

int parse_account_response_metadata(
    AccountResponseMetadata* out,
    const uint8_t* bytes,
    size_t size
) {
    if (!out || !bytes || size != ACCOUNT_RESPONSE_METADATA_SIZE ||
        read_uint(bytes, 2) != 1 || read_uint(bytes + 2, 2) != REQUEST_ACCOUNT ||
        read_uint(bytes + 68, 4) == 0) {
        return -1;
    }

    memcpy(out->account.b, bytes + 4, KEY_SIZE);
    memcpy(out->hash.b, bytes + 36, HASH_SIZE);

    out->size = read_uint(bytes + 68, 4);

    return 0;
}
