/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "operations/package.h"
#include "operations/blob.h"
#include "operations/encrypted_blob.h"
#include "codec/package.h"
#include "networking/context.h"
#include "networking/dispatch.h"
#include <sodium.h>
#include <stdlib.h>

typedef struct PackageLoad {
    // One instance per context, retained across replies until its cleanup handler runs.
    EncryptedBlobLoad blob;

    // The blob reader opens encrypted chunks; parser interprets their package plaintext.
    PackageParser parser;

    // Staging owns provisional contents. destination belongs to the caller.
    Package staging;
    Package* destination;
} PackageLoad;

void package_cleanup(struct Client* cli, ContextID id) {
    PackageLoad* load = cli->net.states[id].context;
    if (load) {
        // Keep completed ciphertext cached; discard partial transfer and provisional plaintext.
        encrypted_blob_clear(cli, id, &load->blob);
        package_destroy(&load->staging);
        sodium_memzero(load, sizeof(*load));
        free(load);
    }
}

int package_get(
    struct Client* cli,
    const Key* owner,
    const HashT* label,
    const Key* key,
    Package* destination,
    size_t max_size,
    ContextID* id
) {
    if (!cli || !owner || !label || !key || !destination || !max_size || !id) {
        return CLIENT_ERR;
    }

    ContextID context = client_new_context(&cli->net);
    if (!valid_id(context)) {
        return CLIENT_CONN_BUSY;
    }

    // Install the handler and its state before sending; the host may reply during request_end.
    ConState* state = &cli->net.states[context];
    state->parser_id = PARSER_ID_PACKAGE;
    PackageLoad* load = calloc(1, sizeof(*load));
    state->context = load;
    if (!load) {
        client_free_context(cli, context);
        return CLIENT_ERR;
    }

    load->blob.key = *key;
    load->destination = destination;
    load->parser.max_size = max_size;
    *id = context;
    return blob_send_get(cli, context, owner, label);
}

static int package_plaintext(
    struct Client* cli,
    ContextID id,
    const uint8_t* bytes,
    size_t size
) {
    PackageLoad* load = cli->net.states[id].context;
    // The content parser copies this authenticated slice into its owned staging allocation.
    return parse_package_contents(&load->parser, &load->staging, bytes, size) == PACKAGE_ERR
        ? CLIENT_ERR : CLIENT_OK;
}

int package_handle_response(struct Client* cli, ContextID id, uint8_t* bytes, uint64_t size) {
    PackageLoad* load = cli->net.states[id].context;
    int result = encrypted_blob_read(cli, id, &load->blob, bytes, size, package_plaintext);
    if (result == CLIENT_OK) {
        return result;
    }
    if (result == CLIENT_PARSE_DONE) {
        if (finish_package(&load->parser) != PACKAGE_DONE) {
            result = CLIENT_ERR;
        } else {
            // Ciphertext hash, final tag, and package contents all passed: transfer ownership.
            package_destroy(load->destination);
            *load->destination = load->staging;
            load->staging = (Package){0};
        }
    }

    client_free_context(cli, id);
    return result;
}
