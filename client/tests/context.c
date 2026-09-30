/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "netwrk/context.h"
#include <assert.h>
#include <stdio.h>

ContextID client_new_context(Networker* net);
void client_free_context(struct Client* cli, ContextID id);

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
    Buffer receive[MAX_CONNS] = {0};
    Buffer send[MAX_CONNS] = {0};
    cli.net.recv_buffers = receive;
    cli.net.send_buffers = send;

    /* Build the free list without relying on the unfinished initializer. */
    for (ContextID id = 1; id < MAX_CONNS; id++) {
        struct DeadConn dead = {
            .id = id,
            .next = id == MAX_CONNS - 1 ? INT16_MAX : id + 1
        };
        memcpy(&cli.net.states[id], &dead, sizeof(dead));
    }
    cli.net.free_head = 1;
    for (ContextID id = 1; id < MAX_CONNS; id++) {
        assert(client_new_context(&cli.net) == id);
        assert(cli.net.states[id].state == CON_IDLE);
    }
    assert(client_new_context(&cli.net) == -1);
    client_free_context(&cli, MAX_CONNS - 1);
    assert(client_new_context(&cli.net) == MAX_CONNS - 1);
    assert(client_new_context(&cli.net) == -1);
}

int main(void)
{
    test_invalid_contexts();
    test_buffer_release();
    test_exhaustion_and_reuse();
    puts("Context tests passed.");
    return 0;
}
