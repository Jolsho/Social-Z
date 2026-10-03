/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "operations/blob.h"
#include "codec/blob.h"
#include "networking/context.h"
#include "networking/dispatch.h"
#include <stdlib.h>

void blob_cleanup(struct Client* cli, ContextID id) {
    BlobTransfer* transfer = cli->net.states[id].operation;
    if (transfer) {
        blob_discard_partial(cli, id, transfer);
        free(transfer);
    }
}

int blob_get(
    struct Client* cli,
    const Key* owner,
    const HashT* label,
    ContextID* id
) {
    if (!cli || !owner || !label || !id) {
        return CLIENT_ERR;
    }

    ContextID context = client_new_context(&cli->net);
    if (!valid_id(context)) {
        return CLIENT_CONN_BUSY;
    }

    ConState* state = &cli->net.states[context];
    state->parser_id = PARSER_ID_BLOB;
    state->operation = calloc(1, sizeof(BlobTransfer));
    if (!state->operation) {
        client_free_context(cli, context);
        return CLIENT_ERR;
    }

    Buffer* buffer = &cli->net.send_buffers[context];
    if (buffer_ensure_min_cap(cli, buffer, BLOB_GET_REQUEST_SIZE) != CLIENT_OK) {
        client_free_context(cli, context);
        return CLIENT_ERR;
    }

    Request request = {.kind = REQUEST_BLOB_GET};
    request.data.blob.owner = *owner;
    request.data.blob.label = *label;
    buffer->size = marshal_blob_request(buffer->b, &request);

    state->state = CON_RECEIVING;
    state->send_owned = true;
    *id = context;

    // Returning from this call does not return ownership of the outgoing buffer.
    send_request(cli, context, buffer);
    return CLIENT_OK;
}

int blob_handle_response(
    struct Client* cli,
    ContextID id,
    uint8_t* bytes,
    uint64_t size
) {
    BlobTransfer* transfer = cli->net.states[id].operation;
    int result = parse_blob(cli, id, transfer, bytes, size);
    if (result != CLIENT_OK) {
        client_free_context(cli, id);
    }

    return result;
}
