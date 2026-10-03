/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "operations/feed_page.h"
#include "operations/blob.h"
#include "operations/encrypted_blob.h"
#include "codec/feed_page.h"
#include "networking/context.h"
#include "networking/dispatch.h"
#include <sodium.h>
#include <stdlib.h>

typedef struct FeedPageLoad {
    EncryptedBlobLoad blob;
    Buffer plaintext; // Owned staging bytes; feed-page parsing needs the complete plaintext.
    uint64_t page_number;
    Feed* destination; // Borrowed Feed stays stable even if its page vector grows.
} FeedPageLoad;

void feed_page_cleanup(struct Client* cli, ContextID id) {
    FeedPageLoad* load = cli->net.states[id].operation;
    if (load) {
        encrypted_blob_clear(cli, id, &load->blob);
        if (load->plaintext.b) {
            sodium_memzero(load->plaintext.b, load->plaintext.cap);
            buffer_pool_push(&cli->pool, load->plaintext.b, load->plaintext.cap);
        }
        sodium_memzero(load, sizeof(*load));
        free(load);
    }
}

static int feed_page_plaintext(
    struct Client* cli,
    ContextID id,
    const uint8_t* bytes,
    size_t size
) {
    FeedPageLoad* load = cli->net.states[id].operation;
    Buffer* plaintext = &load->plaintext;
    if (size > FEED_PAGE_MAX_SIZE - plaintext->size) {
        return CLIENT_ERR;
    }

    // Authenticated slices accumulate here until the whole ciphertext is verified.
    memcpy(plaintext->b + plaintext->size, bytes, size);
    plaintext->size += size;
    return CLIENT_OK;
}

int feed_page_get(
    struct Client* cli,
    const Key* owner,
    const HashT* label,
    const Key* key,
    uint64_t page_number,
    Feed* destination,
    ContextID* id
) {
    if (!cli || !owner || !label || !key || !destination || !id) {
        return CLIENT_ERR;
    }

    ContextID context = client_new_context(&cli->net);
    if (!valid_id(context)) {
        return CLIENT_CONN_BUSY;
    }
    // Prepare all operation state before handing the request to the asynchronous host.
    ConState* state = &cli->net.states[context];
    state->parser_id = PARSER_ID_FEED_PAGE;
    FeedPageLoad* load = calloc(1, sizeof(*load));
    state->operation = load;
    if (!load) {
        client_free_context(cli, context);
        return CLIENT_ERR;
    }

    load->blob.key = *key;
    load->page_number = page_number;
    load->destination = destination;
    if (buffer_ensure_min_cap(cli, &load->plaintext, FEED_PAGE_MAX_SIZE) != CLIENT_OK) {
        client_free_context(cli, context);
        return CLIENT_ERR;
    }

    *id = context;
    return blob_send_get(cli, context, owner, label);
}

int feed_page_handle_response(struct Client* cli, ContextID id, uint8_t* bytes, uint64_t size) {
    FeedPageLoad* load = cli->net.states[id].operation;
    int result = encrypted_blob_read(cli, id, &load->blob, bytes, size, feed_page_plaintext);
    if (result == CLIENT_OK) {
        return result;
    }

    if (result == CLIENT_PARSE_DONE) {
        // Parse and check the requested page number before changing the live Feed.
        FeedPage page = {0};
        if (parse_feed_page(&page, load->plaintext.b, load->plaintext.size) != FEED_PAGE_OK ||
            page.page_number != load->page_number ||
            feed_store_page(load->destination, &page) != FEED_PAGE_OK) {
            result = CLIENT_ERR;
        }
        // feed_store_page moves posts on success; otherwise this releases the rejected staging page.
        feed_page_destroy(&page);
    }

    client_free_context(cli, id);
    return result;
}
