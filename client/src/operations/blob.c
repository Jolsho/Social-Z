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
    BlobTransfer* transfer = cli->net.states[id].context;
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

    ContextID context = networker_new_context(&cli->net);
    if (!valid_id(context)) {
        return CLIENT_CONN_BUSY;
    }

    ConState* state = &cli->net.states[context];
    state->handler_id = HANDLER_ID_BLOB;
    state->context = calloc(1, sizeof(BlobTransfer));
    if (!state->context) {
        networker_free_context(&cli->net, context);
        return CLIENT_ERR;
    }

    *id = context;
    return blob_send_get(cli, context, owner, label);
}

int blob_send_get(struct Client* cli, ContextID id, const Key* owner, const HashT* label) {
    Buffer* buffer = &cli->net.send_buffers[id];
    if (buffer_ensure_min_cap(&cli->pool, buffer, BLOB_GET_REQUEST_SIZE) != CLIENT_OK) {
        networker_free_context(&cli->net, id);
        return CLIENT_ERR;
    }

    Request request = {.kind = REQUEST_BLOB_GET};
    request.data.blob.owner = *owner;
    request.data.blob.label = *label;
    buffer->size = marshal_blob_request(buffer->b, &request);

    return context_send_request(&cli->net, id);
}

int blob_handle_response(
    struct Client* cli,
    ContextID id,
    uint8_t* bytes,
    uint64_t size
) {
    BlobTransfer* transfer = cli->net.states[id].context;
    int result = parse_blob(cli, id, transfer, bytes, size);
    if (result != CLIENT_OK) {
        networker_free_context(&cli->net, id);
    }

    return result;
}
