/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#undef malloc
#undef calloc
#include "client.h"
#include "netwrk/context.h"
#include "netwrk/parsers.h"
#include "sz_common/hash.h"
#include <assert.h>
#include <stdlib.h>

void send_request(struct Client* cli, ContextID id, struct Buffer* buff) {
    (void)cli; (void)id; (void)buff;
    assert(!"Lifecycle tests do not send requests");
}

static size_t allocation_count, fail_at;

void* client_test_malloc(size_t size)
{
    return ++allocation_count == fail_at ? NULL : malloc(size);
}

void* client_test_calloc(size_t count, size_t size)
{
    return ++allocation_count == fail_at ? NULL : calloc(count, size);
}

static void test_setup_and_allocation_failures(void)
{
    struct Client* cli = init_client();
    assert(cli);
    size_t allocations = allocation_count;
    assert(cli->post_feed.index && cli->post_feed.data && !cli->post_feed.size);
    assert(cli->blob_store.table.nodes && cli->net.recv_buffers && cli->net.send_buffers);
    assert(cli->wrld.bvh.root == BVH_NULL && !cli->wrld.bvh.nodes);
    assert(cli->wrld.focused == ENTITY_ID_INVALID);
    KeyPair empty_keys = {0};
    assert(memcmp(&cli->keys, &empty_keys, sizeof(empty_keys)) == 0);
    destroy_client(cli);
    destroy_client(NULL);
    for (size_t failed = 1; failed <= allocations; failed++) {
        allocation_count = 0;
        fail_at = failed;
        assert(init_client() == NULL);
    }
    fail_at = 0;
    cli = init_client();
    assert(cli);
    destroy_client(cli);
}

static void test_public_client_blob_flow(void)
{
    struct Client* cli = init_client();
    assert(cli);
    ContextID id = client_new_context(&cli->net);
    assert(id == 1);
    cli->net.states[id].parser_id = PARSER_ID_USER_DATA;
    assert(client_parse_response(cli, id, NULL, 0) == CLIENT_ERR);
    cli->net.states[id].parser_id = PARSER_ID_POST_FEED;
    assert(client_parse_response(cli, id, NULL, 0) == CLIENT_ERR);
    cli->net.states[id].parser_id = PARSER_ID_BLOB;

    const uint8_t payload[] = {1, 2, 3, 4, 5, 6, 7};
    Hasher hasher = new_hasher();
    hash_update(&hasher, payload, sizeof(payload));
    HashT hash = hash_finalize(&hasher);
    uint64_t total = sizeof(payload);
    uint8_t first[HASH_SIZE + sizeof(total) + 3];
    memcpy(first, hash.b, HASH_SIZE);
    memcpy(first + HASH_SIZE, &total, sizeof(total));
    memcpy(first + HASH_SIZE + sizeof(total), payload, 3);
    assert(client_parse_response(cli, id, first, sizeof(first)) == CLIENT_OK);
    uint8_t last[HASH_SIZE + 4];
    memcpy(last, hash.b, HASH_SIZE);
    memcpy(last + HASH_SIZE, payload + 3, 4);
    assert(client_parse_response(cli, id, last, sizeof(last)) == CLIENT_PARSE_DONE);
    uint8_t result[sizeof(payload)];
    uint64_t size = sizeof(result);
    assert(store_copy_from_item(&cli->blob_store, &hash, 0, result, &size) == STORE_DONE);
    assert(memcmp(result, payload, sizeof(payload)) == 0);

    /* Leave a second assembly and both network buffers owned by the client at shutdown. */
    hash.b[0] ^= 1;
    memcpy(first, hash.b, HASH_SIZE);
    assert(client_parse_response(cli, id, first, sizeof(first)) == CLIENT_OK);
    size_t capacity = 256;
    cli->net.recv_buffers[id].b = buffer_pool_pop(&cli->pool, &capacity);
    cli->net.recv_buffers[id].cap = capacity;
    cli->net.send_buffers[id].b = buffer_pool_pop(&cli->pool, &capacity);
    cli->net.send_buffers[id].cap = capacity;
    assert(cli->net.recv_buffers[id].b && cli->net.send_buffers[id].b);
    destroy_client(cli);
}

int main(void)
{
    test_setup_and_allocation_failures();
    test_public_client_blob_flow();
    return 0;
}
