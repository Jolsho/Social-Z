/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "sz_client/limits.h"
#include "operations/package.h"
#include "codec/package.h"
#include "codec/encrypted_blob.h"
#include "networking/context.h"
#include "sz_common/hash.h"
#include <assert.h>
#include <stdlib.h>

static const Key key = {{1, 2, 3}};
static const Key owner = {{4}};
static const HashT label = {{5}};
static Buffer* pending;

int request_begin(struct Client* cli, ContextID id) {
    (void)cli;
    (void)id;
    return CLIENT_OK;
}

int request_end(struct Client* cli, ContextID id) {
    (void)cli;
    (void)id;
    return CLIENT_OK;
}

void request_abort(struct Client* cli, ContextID id) {
    (void)cli;
    (void)id;
}

int request_write(struct Client* cli, ContextID id, struct Buffer* buffer) {
    assert(!pending && buffer == &cli->net.send_buffers[id]);
    assert(cli->net.states[id].context && cli->net.states[id].send_owned);
    Request request;
    assert(parse_request(&request, buffer->b, buffer->size) == 0);
    assert(request.kind == REQUEST_BLOB_GET);
    assert(memcmp(request.data.blob.owner.b, owner.b, KEY_SIZE) == 0);
    assert(hash_is_equal(&request.data.blob.label, &label));
    pending = buffer;
    return CLIENT_OK;
}

static Buffer encrypted_package(size_t size, size_t chunk_size) {
    uint8_t* data = malloc(size);
    uint8_t* chunk = malloc(chunk_size + ENCRYPTED_CHUNK_OVERHEAD);
    assert(data && chunk);
    for (size_t i = 0; i < size; i++) {
        data[i] = (uint8_t)(i % 251);
    }
    PackageBlob blobs[] = {{3, data}, {size - 3, data + 3}};
    Package package = {.blobs = blobs, .blob_count = 2, .bytes = data, .size = size};
    size_t plaintext = 6 + 2 * sizeof(uint64_t) + size;
    size_t count = (plaintext + chunk_size - 1) / chunk_size;
    Buffer wire = {0};
    wire.cap = ENCRYPTED_BLOB_HEADER_SIZE + plaintext + count * ENCRYPTED_CHUNK_OVERHEAD;
    wire.b = malloc(wire.cap);
    assert(wire.b);
    BlobCrypt crypt = {0};
    assert(encrypt_blob_header(&crypt, wire.b, &key) == 0);
    wire.size = ENCRYPTED_BLOB_HEADER_SIZE;

    PackageMarshaler marshaler = {0};
    int result;
    do {
        size_t written;
        result = marshal_package(&marshaler, &package, chunk, chunk_size, &written);
        assert(result == PACKAGE_MORE || result == PACKAGE_DONE);
        Buffer buffer = {chunk, chunk_size + ENCRYPTED_CHUNK_OVERHEAD, written};
        assert(encrypt_blob_chunk(&crypt, &buffer, result == PACKAGE_DONE) == 0);
        memcpy(wire.b + wire.size, chunk, buffer.size);
        wire.size += buffer.size;
    } while (result != PACKAGE_DONE);

    assert(wire.size == wire.cap);
    free(chunk);
    free(data);
    return wire;
}

static HashT ciphertext_hash(const Buffer* wire) {
    Hasher hasher = new_hasher();
    hash_update(&hasher, wire->b, wire->size);
    return hash_finalize(&hasher);
}

static int deliver(struct Client* cli, ContextID id, const Buffer* wire,
    const HashT* hash, size_t fragment, size_t limit) {
    uint8_t* packet = malloc(HASH_SIZE + sizeof(uint64_t) + fragment);
    assert(packet);
    int result = CLIENT_OK;
    for (size_t offset = 0; offset < limit && result == CLIENT_OK;) {
        size_t prefix = HASH_SIZE;
        memcpy(packet, hash->b, HASH_SIZE);
        if (!offset) {
            uint64_t total = wire->size;
            memcpy(packet + prefix, &total, sizeof(total));
            prefix += sizeof(total);
        }
        size_t length = limit - offset;
        if (length > fragment) {
            length = fragment;
        }
        memcpy(packet + prefix, wire->b + offset, length);
        result = client_parse_response(cli, id, packet, prefix + length);
        memset(packet, 0xff, prefix + length);
        offset += length;
    }
    free(packet);
    return result;
}

static void successful_load(size_t size, size_t encrypted_chunk, size_t fragment) {
    Buffer wire = encrypted_package(size, encrypted_chunk);
    HashT hash = ciphertext_hash(&wire);
    struct Client* cli = init_client();
    assert(cli);
    Package destination = {0};
    ContextID id;
    assert(package_get(cli, &owner, &label, &key, &destination, size + 22, &id) == CLIENT_OK);
    uint8_t request[BLOB_GET_REQUEST_SIZE];
    memcpy(request, pending->b, sizeof(request));
    assert(deliver(cli, id, &wire, &hash, fragment, wire.size) == CLIENT_PARSE_DONE);
    assert(destination.size == size && destination.blob_count == 2);
    assert(destination.blobs[1].bytes == destination.bytes + 3);
    for (size_t i = 0; i < size; i++) {
        assert(destination.bytes[i] == (uint8_t)(i % 251));
    }
    StoreItem* cached = store_get_item(&cli->blob_store, &hash);
    assert(cached && memcmp(cached->b, wire.b, wire.size) == 0);
    assert(!cli->net.states[id].context && cli->net.states[id].release_pending);
    assert(memcmp(pending->b, request, sizeof(request)) == 0);
    client_return_buffer(cli, pending);
    pending = NULL;

    // A cached transfer can answer with just its hash; the existing destination is replaced.
    assert(package_get(cli, &owner, &label, &key, &destination, size + 22, &id) == CLIENT_OK);
    client_return_buffer(cli, pending);
    pending = NULL;
    assert(client_parse_response(cli, id, hash.b, HASH_SIZE) == CLIENT_PARSE_DONE);
    assert(destination.size == size && cli->net.states[id].state == CON_DEAD);
    package_destroy(&destination);
    destroy_client(cli);
    free(wire.b);
}

static void failed_loads(void) {
    Buffer wire = encrypted_package(100, 7);
    HashT hash = ciphertext_hash(&wire);
    for (size_t failure = 0; failure < 4; failure++) {
        struct Client* cli = init_client();
        assert(cli);
        Package destination;
        assert(package_init(&destination, 1, 1) == 0);
        destination.bytes[0] = 0xdd;
        Package before = destination;
        Key supplied_key = key;
        HashT advertised = hash;
        if (failure == 0) supplied_key.b[0] ^= 1;
        if (failure == 1) advertised.b[0] ^= 1;
        size_t max_size = failure == 2 ? 32 : 122;
        ContextID id;
        assert(package_get(cli, &owner, &label, &supplied_key,
            &destination, max_size, &id) == CLIENT_OK);
        size_t limit = failure == 3 ? wire.size - 1 : wire.size;
        int result = deliver(cli, id, &wire, &advertised, 5, limit);
        if (failure == 3) {
            assert(result == CLIENT_OK && cli->net.states[id].context);
            networker_free_context(&cli->net, id);
            assert(!ht_lookup(&cli->blob_store.table, &hash));
        } else {
            assert(result == CLIENT_ERR);
        }
        assert(memcmp(&destination, &before, sizeof(destination)) == 0);
        assert(destination.bytes[0] == 0xdd && !cli->net.states[id].context);
        client_return_buffer(cli, pending);
        pending = NULL;
        package_destroy(&destination);
        destroy_client(cli);
    }
    free(wire.b);
}

static void reject_oversized_transfer(void) {
    struct Client* cli = init_client();
    assert(cli);
    Package destination = {0};
    ContextID id;
    assert(package_get(cli, &owner, &label, &key, &destination, SIZE_MAX, &id) == CLIENT_OK);

    uint8_t packet[HASH_SIZE + sizeof(uint64_t)] = {0};
    uint64_t size = SZ_PACKAGE_CIPHERTEXT_MAX + 1;
    memcpy(packet + HASH_SIZE, &size, sizeof(size));
    uint64_t memory_before = cli->blob_store.mem;
    assert(client_parse_response(cli, id, packet, sizeof(packet)) == CLIENT_ERR);
    assert(cli->blob_store.mem == memory_before && !destination.bytes);
    client_return_buffer(cli, pending);
    pending = NULL;
    destroy_client(cli);
}

int main(void) {
    successful_load(9, 7, 1);
    successful_load(9, 7, 4096);
    successful_load(CRYPT_RECORD_MAX + 100, CRYPT_RECORD_MAX, 4096);
    successful_load(26 * 1024 * 1024, CRYPT_RECORD_MAX, CRYPT_RECORD_MAX);
    failed_loads();
    reject_oversized_transfer();
    return 0;
}
