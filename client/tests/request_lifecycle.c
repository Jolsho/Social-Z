/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "operations/blob.h"
#include "networking/context.h"
#include <assert.h>

static int phase, fail_phase, aborts;
static bool return_early, reply_at_end, return_on_abort;
static Buffer* pending;

int request_begin(struct Client* cli, ContextID id) {
    assert(phase == 0 && cli->net.states[id].context);
    assert(!cli->net.states[id].send_owned);
    phase = 1;
    return fail_phase == phase ? CLIENT_ERR : CLIENT_OK;
}

int request_write(struct Client* cli, ContextID id, Buffer* buffer) {
    assert(phase == 1 && cli->net.states[id].send_owned);
    assert(buffer == &cli->net.send_buffers[id] && buffer->size);
    phase = 2;
    if (fail_phase == phase) {
        return CLIENT_ERR;
    }

    pending = buffer;
    if (return_early) {
        client_return_buffer(cli, pending);
        pending = NULL;
    }
    return CLIENT_OK;
}

int request_end(struct Client* cli, ContextID id) {
    assert(phase == 2 && cli->net.states[id].context);
    phase = 3;
    if (fail_phase == phase) {
        return CLIENT_ERR;
    }

    if (reply_at_end) {
        // A terminal response may free operation state before end returns.
        assert(client_parse_response(cli, id, NULL, 0) == CLIENT_ERR);
    }
    return CLIENT_OK;
}

void request_abort(struct Client* cli, ContextID id) {
    assert(cli->net.states[id].state != CON_DEAD);
    aborts++;
    if (return_on_abort && pending) {
        client_return_buffer(cli, pending);
        pending = NULL;
    }
}

static void test_request(int failure, bool early, bool synchronous) {
    phase = aborts = 0;
    fail_phase = failure;
    return_early = early;
    reply_at_end = synchronous;
    pending = NULL;

    struct Client* cli = init_client();
    assert(cli);
    Key owner = {{1}};
    HashT label = {{2}};
    ContextID id;
    int result = blob_get(cli, &owner, &label, &id);
    assert(result == (failure ? CLIENT_ERR : CLIENT_OK));
    assert(phase == (failure ? failure : 3));
    assert(aborts == (failure > 1 ? 1 : 0));

    if (!failure && !synchronous) {
        assert(cli->net.states[id].context);
        assert(client_request_failed(cli, id) == CLIENT_ERR);
        assert(aborts == 0);
    }
    assert(!cli->net.states[id].context);
    assert(cli->net.states[id].state == CON_DEAD);

    if (pending) {
        assert(cli->net.states[id].release_pending);
        ContextID other = networker_new_context(&cli->net);
        assert(other != id);
        networker_free_context(&cli->net, other);
        client_return_buffer(cli, pending);
        pending = NULL;
    }
    assert(cli->net.free_head == id);
    destroy_client(cli);
}

static void test_cancel(bool release) {
    phase = aborts = fail_phase = 0;
    return_early = reply_at_end = false;
    return_on_abort = release;
    struct Client* cli = init_client();
    assert(cli);
    Key owner = {{1}};
    HashT label = {{2}};
    ContextID id;
    assert(blob_get(cli, &owner, &label, &id) == CLIENT_OK);
    assert(client_cancel_request(cli, id) == CLIENT_OK);
    assert(aborts == 1 && !cli->net.states[id].context);
    assert(client_parse_response(cli, id, NULL, 0) == CLIENT_ERR);
    assert(client_cancel_request(cli, id) == CLIENT_ERR);
    if (pending) {
        assert(cli->net.states[id].release_pending);
        client_return_buffer(cli, pending);
        pending = NULL;
    }
    assert(cli->net.free_head == id);
    destroy_client(cli);
}

int main(void) {
    test_request(1, false, false);
    test_request(2, false, false);
    test_request(3, false, false);
    test_request(3, true, false);
    test_request(0, false, false);
    test_request(0, true, false);
    test_request(0, false, true);
    test_request(0, true, true);
    test_cancel(false);
    test_cancel(true);
    return 0;
}
