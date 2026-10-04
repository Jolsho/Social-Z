/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "networker_setup.h"
#include "networking/context.h"
#include <assert.h>

static Buffer* held[MAX_CONNS];
static ContextID order[16];
static size_t order_size, endings, aborts;
static bool fail_write, return_immediately;

typedef struct Transfer {
    size_t chunks, cleanups;
} Transfer;

int request_begin(struct Client* cli, ContextID id) {
    (void)cli;
    (void)id;
    return CLIENT_OK;
}

int request_write(struct Client* cli, ContextID id, Buffer* buffer) {
    assert(buffer == &cli->net.send_buffers[id] && !held[id]);
    if (fail_write) {
        return CLIENT_ERR;
    }
    if (return_immediately) {
        client_return_buffer(cli, buffer);
    } else {
        held[id] = buffer;
    }
    return CLIENT_OK;
}

int request_end(struct Client* cli, ContextID id) {
    (void)cli;
    (void)id;
    endings++;
    return CLIENT_OK;
}

void request_abort(struct Client* cli, ContextID id) {
    (void)cli;
    (void)id;
    aborts++;
}

static void cleanup(struct Client* cli, ContextID id) {
    Transfer* transfer = cli->net.states[id].context;
    transfer->cleanups++;
}

static int send_next(struct Client* cli, ContextID id) {
    ConState* state = &cli->net.states[id];
    Transfer* transfer = state->context;
    size_t before = order_size;
    client_poll(cli);
    assert(order_size == before); // Host callbacks cannot recursively advance the list.

    if (!transfer->chunks) {
        assert(request_begin(cli, id) == CLIENT_OK);
        state->request_started = true;
    }
    order[order_size++] = id;
    transfer->chunks++;

    Buffer* buffer = &cli->net.send_buffers[id];
    assert(buffer_ensure_min_cap(&cli->pool, buffer, 1) == CLIENT_OK);
    buffer->b[0] = transfer->chunks;
    buffer->size = 1;
    state->send_owned = true;
    if (request_write(cli, id, buffer) != CLIENT_OK) {
        state->send_owned = false;
        return CLIENT_ERR;
    }

    if (transfer->chunks == 2) {
        // Sending is finished even though a response and buffer return may still be pending.
        networker_stop_sending(&cli->net, id);
        return request_end(cli, id);
    }
    return CLIENT_OK;
}

static const HandlerEntry handlers[] = {{.cleanup = cleanup, .sender = send_next}};

static void setup(struct Client* cli) {
    assert(init_test_networker(cli) == CLIENT_OK);
    assert(buffer_pool_init(&cli->pool, 256, 4, 4096, 1, 65536, 1) == 0);
    cli->net.handlers = handlers;
    cli->net.handlers_count = 1;
    order_size = endings = aborts = 0;
    fail_write = return_immediately = false;
}

static ContextID start(struct Client* cli, Transfer* transfer) {
    ContextID id = networker_new_context(&cli->net);
    assert(id > 0);
    cli->net.states[id].context = transfer;
    networker_start_sending(&cli->net, id);
    networker_start_sending(&cli->net, id); // Duplicate registration must not create a cycle.
    return id;
}

static void return_buffer(struct Client* cli, ContextID id) {
    Buffer* buffer = held[id];
    held[id] = NULL;
    client_return_buffer(cli, buffer);
}

static void test_rotation(void) {
    struct Client cli = {0};
    setup(&cli);
    Transfer first = {0}, second = {0};
    ContextID a = start(&cli, &first);
    ContextID b = start(&cli, &second);

    assert(client_poll(&cli) && first.chunks == 1 && second.chunks == 0);
    assert(client_poll(&cli) && second.chunks == 1);
    assert(client_poll(&cli) && order_size == 2); // Waiting context consumes one turn.
    return_buffer(&cli, b);
    assert(second.chunks == 1); // Returning memory does not send.
    assert(client_poll(&cli) && second.chunks == 2 && endings == 1);
    assert(!cli.net.states[b].sending);

    return_buffer(&cli, a);
    assert(!client_poll(&cli) && first.chunks == 2 && endings == 2);
    assert(!cli.net.send_head && !cli.net.send_cursor);
    assert(!client_poll(&cli) && order_size == 4);
    assert(order[0] == a && order[1] == b && order[2] == b && order[3] == a);

    // Cancelling after sending still waits for retained buffers before context reuse.
    assert(client_cancel_request(&cli, a) == CLIENT_OK);
    assert(first.cleanups == 1 && cli.net.states[a].release_pending);
    return_buffer(&cli, a);
    return_buffer(&cli, b);
    destroy_networker(&cli.net);
    assert(first.cleanups == 1 && second.cleanups == 1);
    buffer_pool_destroy(&cli.pool);
}

static void test_removal_and_reuse(void) {
    struct Client cli = {0};
    setup(&cli);
    Transfer transfers[4] = {0};
    ContextID a = start(&cli, &transfers[0]);
    ContextID b = start(&cli, &transfers[1]);
    ContextID c = start(&cli, &transfers[2]);

    assert(client_cancel_request(&cli, b) == CLIENT_OK); // Middle of c -> b -> a.
    assert(client_cancel_request(&cli, a) == CLIENT_OK); // Cursor and tail.
    ContextID reused = start(&cli, &transfers[3]);
    assert(reused == a);
    assert(client_cancel_request(&cli, c) == CLIENT_OK); // Cursor moves to reused ID.
    return_immediately = true;
    assert(client_poll(&cli) && transfers[3].chunks == 1);
    assert(!client_poll(&cli) && transfers[3].chunks == 2);
    assert(!cli.net.send_head && !cli.net.send_cursor);
    destroy_networker(&cli.net);
    for (size_t i = 0; i < 4; i++) {
        assert(transfers[i].cleanups == 1);
    }
    buffer_pool_destroy(&cli.pool);
}

static void test_failure_and_shutdown(void) {
    struct Client cli = {0};
    setup(&cli);
    Transfer failed = {0}, pending = {0};
    ContextID id = start(&cli, &failed);
    fail_write = true;
    assert(!client_poll(&cli) && failed.cleanups == 1 && aborts == 1);
    assert(cli.net.states[id].state == CON_DEAD);

    start(&cli, &pending);
    destroy_networker(&cli.net); // Remove a sender that has not started yet.
    assert(pending.cleanups == 1 && !client_poll(&cli));
    buffer_pool_destroy(&cli.pool);
}

int main(void) {
    assert(!client_poll(NULL));
    test_rotation();
    test_removal_and_reuse();
    test_failure_and_shutdown();
    return 0;
}
