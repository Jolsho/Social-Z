/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#undef calloc
#include "client.h"
#include "networking/context.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static int calloc_calls, fail_calloc_at;

void* context_test_calloc(size_t count, size_t size)
{
    return ++calloc_calls == fail_calloc_at ? NULL : calloc(count, size);
}

static void test_invalid_contexts(void)
{
    const ContextID invalid[] = {-1, 0, MAX_CONNS, MAX_CONNS + 1, INT16_MAX};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        ContextID id = invalid[i];
        assert(!valid_id(id));
        /* No Client memory is needed when the ID is rejected first. */
        assert(context_release_recv_buffer(NULL, id) == CLIENT_INVALID_ID);
        assert(context_release_send_buffer(NULL, id) == CLIENT_INVALID_ID);
        assert(client_parse_response(NULL, id, NULL, 0) == CLIENT_INVALID_ID);
    }
    assert(valid_id(1));
    assert(valid_id(MAX_CONNS - 1));
}

static void test_buffer_release(void)
{
    struct Client cli = {0};
    Buffer receive[MAX_CONNS] = {0};
    Buffer send[MAX_CONNS] = {0};
    cli.net.recv_buffers = receive;
    cli.net.send_buffers = send;
    assert(buffer_pool_init(&cli.pool, 256, 2, 4096, 1, 65536, 1) == 0);

    const ContextID ids[] = {1, MAX_CONNS - 1};
    for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); i++) {
        ContextID id = ids[i];
        size_t recv_cap = 256, send_cap = 256;
        receive[id].b = buffer_pool_pop(&cli.pool, &recv_cap);
        send[id].b = buffer_pool_pop(&cli.pool, &send_cap);
        assert(receive[id].b && send[id].b);
        receive[id].cap = (uint32_t)recv_cap;
        send[id].cap = (uint32_t)send_cap;
        receive[id].size = send[id].size = 8;
        assert(cli.pool.buckets[0].available == 0);
        assert(context_release_recv_buffer(&cli, id) == CLIENT_OK);
        assert(context_release_send_buffer(&cli, id) == CLIENT_OK);
        assert(!receive[id].b && !receive[id].cap && !receive[id].size);
        assert(!send[id].b && !send[id].cap && !send[id].size);
        assert(cli.pool.buckets[0].available == 2);
        assert(context_release_recv_buffer(&cli, id) == CLIENT_OK);
        assert(context_release_send_buffer(&cli, id) == CLIENT_OK);
        assert(cli.pool.buckets[0].available == 2);
    }
    buffer_pool_destroy(&cli.pool);
}

static void test_exhaustion_and_reuse(void)
{
    struct Client cli = {0};
    memset(&cli.net, 0xa5, sizeof(cli.net));
    assert(init_networker(&cli.net) == CLIENT_OK);
    for (ContextID id = 1; id < MAX_CONNS; id++) {
        assert(client_new_context(&cli.net) == id);
        assert(cli.net.states[id].state == CON_IDLE);
    }
    assert(client_new_context(&cli.net) == -1);
    client_free_context(&cli, MAX_CONNS - 1);
    client_free_context(&cli, MAX_CONNS - 1);
    assert(client_new_context(&cli.net) == MAX_CONNS - 1);
    assert(client_new_context(&cli.net) == -1);
    destroy_networker(&cli);
    destroy_networker(&cli);
    assert(client_new_context(&cli.net) == -1);
    assert(!cli.net.recv_buffers && !cli.net.send_buffers);
}

static void test_setup_failures_and_dispatch(void)
{
    struct Client cli = {0};
    assert(init_networker(NULL) == CLIENT_ERR);
    assert(client_new_context(NULL) == -1);
    for (int failed = 1; failed <= 2; failed++) {
        calloc_calls = 0;
        fail_calloc_at = failed;
        assert(init_networker(&cli.net) == CLIENT_ERR);
        assert(!cli.net.recv_buffers && !cli.net.send_buffers);
        assert(client_new_context(&cli.net) == -1);
        destroy_networker(&cli);
    }
    fail_calloc_at = 0;
    assert(init_networker(&cli.net) == CLIENT_OK);
    assert(client_parse_response(NULL, 1, NULL, 0) == CLIENT_ERR);
    assert(client_new_context(&cli.net) == 1);
    assert(client_parse_response(&cli, 1, NULL, 0) == CLIENT_ERR);
    const HandlerEntry empty[] = {{0}};
    cli.net.handlers = empty;
    cli.net.handlers_count = 1;
    assert(client_parse_response(&cli, 1, NULL, 0) == CLIENT_ERR);
    destroy_networker(&cli);
}

static void test_shutdown_returns_buffers(void)
{
    struct Client cli = {0};
    assert(init_networker(&cli.net) == CLIENT_OK);
    assert(buffer_pool_init(&cli.pool, 256, 2, 4096, 1, 65536, 1) == 0);
    ContextID id = client_new_context(&cli.net);
    size_t capacity = 256;
    cli.net.recv_buffers[id].b = buffer_pool_pop(&cli.pool, &capacity);
    cli.net.recv_buffers[id].cap = capacity;
    cli.net.send_buffers[id].b = buffer_pool_pop(&cli.pool, &capacity);
    cli.net.send_buffers[id].cap = capacity;
    assert(cli.pool.buckets[0].available == 0);
    destroy_networker(&cli);
    assert(cli.pool.buckets[0].available == 2);
    buffer_pool_destroy(&cli.pool);
}

typedef struct TestOperation {
    size_t replies;
    size_t cleanups;
} TestOperation;

static void cleanup_operation(struct Client* cli, ContextID id) {
    TestOperation* operation = cli->net.states[id].context;
    assert(operation);
    operation->cleanups++;
}

static int parse_operation(struct Client* cli, ContextID id, uint8_t* bytes, uint64_t size) {
    (void)bytes;
    TestOperation* operation = cli->net.states[id].context;
    assert(operation);
    operation->replies++;
    if (size) {
        client_free_context(cli, id);
        return CLIENT_PARSE_DONE;
    }
    return CLIENT_OK;
}

static void test_operation_cleanup(void) {
    struct Client cli = {0};
    assert(init_networker(&cli.net) == CLIENT_OK);
    assert(buffer_pool_init(&cli.pool, 256, 1, 4096, 1, 65536, 1) == 0);
    const HandlerEntry handlers[] = {{0}, {parse_operation, cleanup_operation}};
    cli.net.handlers = handlers;
    cli.net.handlers_count = 2;

    TestOperation operation = {0};
    ContextID id = client_new_context(&cli.net);
    cli.net.states[id].handler_id = 1;
    cli.net.states[id].context = &operation;
    assert(client_parse_response(&cli, id, NULL, 0) == CLIENT_OK);
    client_free_context(&cli, id);
    client_free_context(&cli, id);
    assert(operation.replies == 1 && operation.cleanups == 1);
    assert(!cli.net.states[id].context);

    // Completion cleans operation state immediately, while the host still owns the send buffer.
    id = client_new_context(&cli.net);
    cli.net.states[id].handler_id = 1;
    cli.net.states[id].context = &operation;
    size_t capacity = 256;
    Buffer* buffer = &cli.net.send_buffers[id];
    buffer->b = buffer_pool_pop(&cli.pool, &capacity);
    assert(buffer->b);
    buffer->cap = capacity;
    cli.net.states[id].send_owned = true;
    assert(client_parse_response(&cli, id, NULL, 1) == CLIENT_PARSE_DONE);
    assert(operation.replies == 2 && operation.cleanups == 2);
    assert(!cli.net.states[id].context && cli.net.states[id].release_pending);
    assert(cli.pool.buckets[0].available == 0);
    client_free_context(&cli, id);
    client_return_buffer(&cli, buffer);
    assert(operation.cleanups == 2 && cli.pool.buckets[0].available == 1);

    // Shutdown follows the same registered cleanup path.
    id = client_new_context(&cli.net);
    cli.net.states[id].handler_id = 1;
    cli.net.states[id].context = &operation;
    destroy_networker(&cli);
    assert(operation.cleanups == 3);
    buffer_pool_destroy(&cli.pool);
}

int main(void)
{
    test_invalid_contexts();
    test_buffer_release();
    test_exhaustion_and_reuse();
    test_setup_failures_and_dispatch();
    test_shutdown_returns_buffers();
    test_operation_cleanup();
    puts("Context tests passed.");
    return 0;
}

// These tests exercise context/storage internals without submitting host requests.
int request_begin(struct Client* cli, ContextID id) {
    (void)cli;
    (void)id;
    return CLIENT_ERR;
}

int request_write(struct Client* cli, ContextID id, struct Buffer* buffer) {
    (void)cli;
    (void)id;
    (void)buffer;
    return CLIENT_ERR;
}

int request_end(struct Client* cli, ContextID id) {
    (void)cli;
    (void)id;
    return CLIENT_ERR;
}

void request_abort(struct Client* cli, ContextID id) {
    (void)cli;
    (void)id;
}
