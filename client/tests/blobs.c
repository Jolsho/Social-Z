/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "sz_common/hash.h"
#include <assert.h>
#include <stdlib.h>

int parse_blob(struct Client*, ContextID, uint8_t*, uint64_t);
void client_free_context(struct Client*, ContextID);
static Parser parsers[] = {parse_blob};

static void setup(struct Client* cli)
{
    memset(cli, 0, sizeof(*cli));
    assert(store_setup(&cli->blob_store, 200000) == STORE_OK);
    assert(buffer_pool_init(&cli->pool, 64, 4, 256, 1, 4096, 1) == 0);
    cli->net.parsers = parsers;
    cli->net.parsers_count = 1;
}

static void cleanup(struct Client* cli)
{
    store_destroy(&cli->blob_store);
    assert(cli->pool.buckets[0].available == 4);
    assert(cli->pool.buckets[1].available == 1);
    assert(cli->pool.buckets[2].available == 1);
    buffer_pool_destroy(&cli->pool);
}

static HashT blob_hash(const uint8_t* bytes, size_t size)
{
    Hasher hasher = new_hasher();
    hash_update(&hasher, bytes, size);
    return hash_finalize(&hasher);
}

static int chunk(struct Client* cli, ContextID id, HashT hash, uint64_t total,
                 bool first, const uint8_t* bytes, size_t size)
{
    uint8_t packet[HASH_SIZE + sizeof(uint64_t) + 257];
    assert(size <= 257);
    size_t header = HASH_SIZE;
    memcpy(packet, hash.b, HASH_SIZE);
    if (first) {
        memcpy(packet + header, &total, sizeof(total));
        header += sizeof(total);
    }
    memcpy(packet + header, bytes, size);
    int result = client_parse_response(cli, id, packet, header + size);
    memset(packet, 0xff, sizeof(packet)); /* Responses are borrowed and immediately reused. */
    return result;
}

static void test_large_blob(void)
{
    struct Client cli;
    setup(&cli);
    const size_t size = 70003; /* Larger than the client's normal 64 KiB receive buffer. */
    uint8_t* bytes = malloc(size);
    assert(bytes);
    for (size_t i = 0; i < size; i++) bytes[i] = i % 251;
    HashT hash = blob_hash(bytes, size);
    assert(chunk(&cli, 1, hash, size, true, bytes, 100) == CLIENT_OK);
    StoreItem* item = ht_lookup(&cli.blob_store.table, &hash);
    assert(item && item->received == 100 && item->size == size);
    assert(!item->pool && item->capacity == size);
    uint8_t output[7];
    uint64_t count = sizeof(output);
    assert(store_copy_from_item(&cli.blob_store, &hash, 0, output, &count) == STORE_ERR);
    for (size_t offset = 100; offset < size;) {
        size_t length = size - offset;
        if (length > 137) length = 137;
        int expected = offset + length == size ? CLIENT_PARSE_DONE : CLIENT_OK;
        assert(chunk(&cli, 1, hash, size, false, bytes + offset, length) == expected);
        offset += length;
    }
    assert(item->received == size && !cli.net.states[1].blob_active);
    assert(memcmp(item->b, bytes, size) == 0 && cli.blob_store.mem == size);
    assert(store_copy_from_item(&cli.blob_store, &hash, 0, output, &count) == STORE_OK);
    assert(memcmp(output, bytes, sizeof(output)) == 0);
    assert(chunk(&cli, 2, hash, size, true, bytes, 3) == CLIENT_PARSE_DONE);
    assert(cli.blob_store.table.size == 1 && cli.blob_store.mem == size);
    free(bytes);
    cleanup(&cli);
}

static void test_bounds_and_final_hash(void)
{
    struct Client cli;
    setup(&cli);
    uint8_t bytes[] = {1, 2, 3, 4, 5, 6, 7};
    HashT hash = blob_hash(bytes, sizeof(bytes));
    assert(parse_blob(NULL, 0, NULL, 0) == CLIENT_INVALID_ID);
    assert(parse_blob(&cli, 1, NULL, 0) == CLIENT_ERR);
    for (size_t length = 0; length < HASH_SIZE + sizeof(uint64_t); length++)
        assert(parse_blob(&cli, 1, hash.b, length) == CLIENT_SMALL_BUFFER);
    const uint64_t invalid[] = {0, 2, UINT64_MAX, 200000};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++)
        assert(chunk(&cli, 1, hash, invalid[i], true, bytes, 3) == CLIENT_ERR);
    assert(!cli.blob_store.mem && !cli.blob_store.table.size);

    assert(chunk(&cli, 1, hash, 7, true, bytes, 3) == CLIENT_OK);
    StoreItem* item = ht_lookup(&cli.blob_store.table, &hash);
    assert(item && item->pool == &cli.pool && item->capacity == 64);
    assert(cli.pool.buckets[0].available == 3 && cli.blob_store.mem == 64);
    assert(chunk(&cli, 1, hash, 7, false, bytes, 0) == CLIENT_SMALL_BUFFER);
    assert(chunk(&cli, 2, hash, 7, false, bytes + 3, 4) == CLIENT_CONN_BUSY);
    assert(chunk(&cli, 1, hash, 7, false, bytes, 5) == CLIENT_ERR);
    assert(!cli.blob_store.mem && !cli.net.states[1].blob_active);

    assert(chunk(&cli, 1, hash, 7, true, bytes, 3) == CLIENT_OK);
    bytes[6] ^= 1;
    assert(chunk(&cli, 1, hash, 7, false, bytes + 3, 4) == CLIENT_ERR);
    assert(!cli.blob_store.mem && !cli.blob_store.table.size);
    bytes[6] ^= 1;
    assert(chunk(&cli, 1, hash, 7, true, bytes, 7) == CLIENT_PARSE_DONE);
    cleanup(&cli);
}

static void test_interleaving_eviction_and_cancel(void)
{
    struct Client cli;
    Buffer receive[MAX_CONNS] = {0}, send[MAX_CONNS] = {0};
    setup(&cli);
    cli.net.recv_buffers = receive;
    cli.net.send_buffers = send;
    uint8_t a[] = {1, 2, 3, 4}, b[] = {5, 6, 7, 8};
    HashT ha = blob_hash(a, sizeof(a)), hb = blob_hash(b, sizeof(b));
    assert(chunk(&cli, 1, ha, 4, true, a, 2) == CLIENT_OK);
    assert(chunk(&cli, 2, hb, 4, true, b, 2) == CLIENT_OK);
    assert(chunk(&cli, 1, hb, 4, false, b + 2, 2) == CLIENT_ERR);
    assert(chunk(&cli, 2, hb, 4, false, b + 2, 2) == CLIENT_PARSE_DONE);
    client_free_context(&cli, 1);
    assert(!ht_lookup(&cli.blob_store.table, &ha) && cli.blob_store.mem == 64);

    cli.blob_store.mem_max = 64;
    assert(chunk(&cli, 1, ha, 4, true, a, 2) == CLIENT_OK);
    /* A new blob evicts this partial assembly; its continuation must not become a first chunk. */
    store_erase_item(&cli.blob_store, &hb);
    assert(chunk(&cli, 2, hb, 4, true, b, 2) == CLIENT_OK);
    assert(!ht_lookup(&cli.blob_store.table, &ha));
    assert(chunk(&cli, 1, ha, 4, false, a + 2, 2) == CLIENT_ERR);
    assert(chunk(&cli, 2, hb, 4, false, b + 2, 2) == CLIENT_PARSE_DONE);
    cleanup(&cli);
}

static void test_pool_limit_and_exhaustion(void)
{
    struct Client cli;
    setup(&cli);
    uint8_t bytes[4097] = {0};
    HashT pooled = blob_hash(bytes, 4096), large = blob_hash(bytes, sizeof(bytes));
    assert(chunk(&cli, 1, pooled, 4096, true, bytes, 3) == CLIENT_OK);
    StoreItem* item = ht_lookup(&cli.blob_store.table, &pooled);
    assert(item && item->pool == &cli.pool && item->capacity == 4096);
    assert(cli.pool.buckets[2].available == 0);
    HashT unavailable = pooled;
    unavailable.b[0] ^= 1;
    assert(chunk(&cli, 2, unavailable, 4096, true, bytes, 3) == CLIENT_ERR);
    assert(cli.blob_store.table.size == 1 && cli.blob_store.mem == 4096);
    assert(!cli.net.states[2].blob_active);
    assert(chunk(&cli, 2, large, sizeof(bytes), true, bytes, 3) == CLIENT_OK);
    item = ht_lookup(&cli.blob_store.table, &large);
    assert(item && !item->pool && item->capacity == sizeof(bytes));
    cleanup(&cli);
}

int main(void)
{
    test_large_blob();
    test_bounds_and_final_hash();
    test_interleaving_eviction_and_cancel();
    test_pool_limit_and_exhaustion();
    return 0;
}
