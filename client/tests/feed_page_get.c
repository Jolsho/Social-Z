/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "operations/feed_page.h"
#include "codec/feed_page.h"
#include "codec/encrypted_blob.h"
#include "networking/context.h"
#include "sz_common/hash.h"
#include <assert.h>
#include <stdlib.h>

static const Key key = {{1, 2, 3}};
static const Key owner = {{4}};
static const HashT label = {{5}};
static Buffer* pending[MAX_CONNS];

int request_begin(struct Client* cli, ContextID id) {
    (void)cli;
    (void)id;
    return CLIENT_OK;
}

int request_end(struct Client* cli, ContextID id) {
    (void)cli;
    (void)id;
    return CLIENT_OK;
}

void request_abort(struct Client* cli, ContextID id) {
    (void)cli;
    (void)id;
}

int request_write(struct Client* cli, ContextID id, struct Buffer* buffer) {
    assert(!pending[id] && buffer == &cli->net.send_buffers[id]);
    assert(cli->net.states[id].operation && cli->net.states[id].send_owned);
    Request request;
    assert(parse_request(&request, buffer->b, buffer->size) == 0);
    assert(request.kind == REQUEST_BLOB_GET);
    assert(memcmp(request.data.blob.owner.b, owner.b, KEY_SIZE) == 0);
    pending[id] = buffer;
    return CLIENT_OK;
}

static Buffer encrypted_feed(uint64_t page_number, bool malformed, bool oversized) {
    uint8_t* plaintext = calloc(FEED_PAGE_MAX_SIZE + 1, 1);
    assert(plaintext);
    FeedPage page = {0};
    assert(feed_page_init(&page, page_number) == FEED_PAGE_OK);
    uint8_t post[POST_SIZE] = {0xaa};
    uint32_t count = 1;
    memcpy(post + POST_BLOB_COUNT_OFFSET, &count, sizeof(count));
    assert(feed_page_append_posts(&page, post, sizeof(post)) == FEED_PAGE_OK);
    size_t size;
    assert(marshal_feed_page(plaintext, FEED_PAGE_MAX_SIZE, &size, &page) == FEED_PAGE_OK);
    feed_page_destroy(&page);
    if (malformed) {
        plaintext[1] = 2;
    }
    if (oversized) {
        size = FEED_PAGE_MAX_SIZE + 1;
    }

    size_t chunk_size = oversized ? CRYPT_RECORD_MAX : 7;
    size_t count_chunks = (size + chunk_size - 1) / chunk_size;
    Buffer wire = {0};
    wire.cap = ENCRYPTED_BLOB_HEADER_SIZE + size + count_chunks * ENCRYPTED_CHUNK_OVERHEAD;
    wire.b = malloc(wire.cap);
    uint8_t* chunk = malloc(chunk_size + ENCRYPTED_CHUNK_OVERHEAD);
    assert(wire.b && chunk);
    BlobCrypt crypt = {0};
    assert(encrypt_blob_header(&crypt, wire.b, &key) == 0);
    wire.size = ENCRYPTED_BLOB_HEADER_SIZE;
    for (size_t offset = 0; offset < size;) {
        size_t length = size - offset;
        if (length > chunk_size) {
            length = chunk_size;
        }
        memcpy(chunk, plaintext + offset, length);
        Buffer buffer = {chunk, chunk_size + ENCRYPTED_CHUNK_OVERHEAD, length};
        assert(encrypt_blob_chunk(&crypt, &buffer, offset + length == size) == 0);
        memcpy(wire.b + wire.size, chunk, buffer.size);
        wire.size += buffer.size;
        offset += length;
    }
    assert(wire.size == wire.cap);
    free(chunk);
    free(plaintext);
    return wire;
}

static HashT ciphertext_hash(const Buffer* wire) {
    Hasher hasher = new_hasher();
    hash_update(&hasher, wire->b, wire->size);
    return hash_finalize(&hasher);
}

static int reply(struct Client* cli, ContextID id, const Buffer* wire,
    const HashT* hash, size_t offset, size_t length) {
    uint8_t* packet = malloc(HASH_SIZE + sizeof(uint64_t) + length);
    assert(packet);
    memcpy(packet, hash->b, HASH_SIZE);
    size_t prefix = HASH_SIZE;
    if (!offset) {
        uint64_t total = wire->size;
        memcpy(packet + prefix, &total, sizeof(total));
        prefix += sizeof(total);
    }
    memcpy(packet + prefix, wire->b + offset, length);
    int result = client_parse_response(cli, id, packet, prefix + length);
    memset(packet, 0xff, prefix + length);
    free(packet);
    return result;
}

static void return_request(struct Client* cli, ContextID id) {
    assert(!cli->net.states[id].operation);
    client_return_buffer(cli, pending[id]);
    pending[id] = NULL;
}

static void successful_load(size_t fragment) {
    Buffer wire = encrypted_feed(7, false, false);
    HashT hash = ciphertext_hash(&wire);
    struct Client* cli = init_client();
    assert(cli);
    cli->feed.current_page_number = 7;
    ContextID id;
    assert(feed_page_get(cli, &owner, &label, &key, 7, &cli->feed, &id) == CLIENT_OK);
    for (size_t offset = 0; offset < wire.size;) {
        size_t length = wire.size - offset;
        if (length > fragment) {
            length = fragment;
        }
        int result = reply(cli, id, &wire, &hash, offset, length);
        offset += length;
        assert(result == (offset == wire.size ? CLIENT_PARSE_DONE : CLIENT_OK));
        assert(cli->feed.size == (offset == wire.size ? 1 : 0));
    }
    FeedPage* page = feed_find_page(&cli->feed, 7);
    assert(page && page->size == 1 && page->posts[0] == 0xaa);
    assert(cli->feed.current_page_number == 7);
    StoreItem* cached = store_get_item(&cli->blob_store, &hash);
    assert(cached && memcmp(cached->b, wire.b, wire.size) == 0);
    assert(cli->net.states[id].release_pending);
    return_request(cli, id);

    // Loading the cached page replaces its contents without appending a duplicate page.
    assert(feed_page_get(cli, &owner, &label, &key, 7, &cli->feed, &id) == CLIENT_OK);
    assert(client_parse_response(cli, id, hash.b, HASH_SIZE) == CLIENT_PARSE_DONE);
    assert(cli->feed.size == 1 && feed_find_page(&cli->feed, 7)->posts[0] == 0xaa);
    return_request(cli, id);
    destroy_client(cli);
    free(wire.b);
}

static void failed_loads(void) {
    for (size_t failure = 0; failure < 6; failure++) {
        Buffer wire = encrypted_feed(7, failure == 3, failure == 5);
        HashT hash = ciphertext_hash(&wire);
        struct Client* cli = init_client();
        assert(cli);
        FeedPage old = {0};
        assert(feed_page_init(&old, 7) == FEED_PAGE_OK);
        assert(feed_store_page(&cli->feed, &old) == FEED_PAGE_OK);
        Feed before = cli->feed;
        uint8_t* old_posts = feed_find_page(&cli->feed, 7)->posts;
        Key supplied = key;
        if (failure == 0) {
            supplied.b[0] ^= 1;
        }
        if (failure == 2) {
            hash.b[0] ^= 1;
        }
        ContextID id;
        assert(feed_page_get(cli, &owner, &label, &supplied,
            failure == 1 ? 8 : 7, &cli->feed, &id) == CLIENT_OK);
        size_t limit = failure == 4 ? wire.size - 1 : wire.size;
        int result = reply(cli, id, &wire, &hash, 0, limit);
        if (failure == 4) {
            assert(result == CLIENT_OK);
            client_free_context(cli, id);
            assert(!ht_lookup(&cli->blob_store.table, &hash));
        } else {
            assert(result == CLIENT_ERR);
        }
        assert(memcmp(&cli->feed, &before, sizeof(before)) == 0);
        assert(feed_find_page(&cli->feed, 7)->posts == old_posts);
        return_request(cli, id);
        destroy_client(cli);
        free(wire.b);
    }
}

static void overlapping_loads(void) {
    Buffer a = encrypted_feed(7, false, false), b = encrypted_feed(9, false, false);
    HashT ha = ciphertext_hash(&a), hb = ciphertext_hash(&b);
    struct Client* cli = init_client();
    assert(cli);
    ContextID first, second;
    HashT other_label = {{9}};
    assert(feed_page_get(cli, &owner, &label, &key, 7, &cli->feed, &first) == CLIENT_OK);
    assert(feed_page_get(cli, &owner, &other_label, &key, 9, &cli->feed, &second) == CLIENT_OK);
    assert(first != second);
    assert(reply(cli, first, &a, &ha, 0, 1) == CLIENT_OK);
    assert(reply(cli, second, &b, &hb, 0, 1) == CLIENT_OK);
    assert(reply(cli, second, &b, &hb, 1, b.size - 1) == CLIENT_PARSE_DONE);
    assert(cli->feed.size == 1 && feed_find_page(&cli->feed, 9));
    assert(reply(cli, first, &a, &ha, 1, a.size - 1) == CLIENT_PARSE_DONE);
    assert(cli->feed.size == 2 && feed_find_page(&cli->feed, 7));
    return_request(cli, second);
    return_request(cli, first);
    destroy_client(cli);
    free(a.b);
    free(b.b);
}

int main(void) {
    successful_load(1);
    successful_load(4096);
    failed_loads();
    overlapping_loads();
    return 0;
}
