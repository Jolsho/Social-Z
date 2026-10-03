/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "client.h"
#include "operations/login.h"
#include "networking/context.h"
#include "sz_common/hash.h"
#include <sodium.h>
#include <assert.h>

static const uint8_t password[] = "my test password";
static KeyPair identity;
static Key owner_key;
static AccountHeader account_header;
static uint8_t blob[LOGIN_BLOB_SIZE];

typedef struct Host {
    struct Client* cli;
    ContextID id;
    uint8_t request[6 + ACCOUNT_USERNAME_MAX];
    size_t size, calls;
    bool fail, synchronous, retain;
    Buffer* pending;
    int response;
} Host;


static HashT blob_hash(const uint8_t* bytes) {
    Hasher hasher = new_hasher();
    hash_update(&hasher, bytes, LOGIN_BLOB_SIZE);
    return hash_finalize(&hasher);
}

static void reply_metadata(uint8_t reply[ACCOUNT_RESPONSE_METADATA_SIZE], const uint8_t* bytes, const Key* account) {
    AccountResponseMetadata metadata = {
        .account = *account,
        .hash = blob_hash(bytes),
        .size = LOGIN_BLOB_SIZE,
    };
    marshal_account_response_metadata(reply, &metadata);
}

static int deliver_blob(Host* host, const uint8_t* bytes, size_t chunk_size) {
    HashT hash = blob_hash(bytes);
    int r = CLIENT_OK;
    for (size_t offset = 0; offset < LOGIN_BLOB_SIZE && r == CLIENT_OK;) {
        uint8_t chunk[HASH_SIZE + sizeof(uint64_t) + LOGIN_BLOB_SIZE];
        memcpy(chunk, hash.b, HASH_SIZE);
        size_t prefix = HASH_SIZE;
        if (!offset) {
            uint64_t size = LOGIN_BLOB_SIZE;
            memcpy(chunk + prefix, &size, sizeof(size));
            prefix += sizeof(size);
        }
        size_t n = LOGIN_BLOB_SIZE - offset;
        if (n > chunk_size) n = chunk_size;
        memcpy(chunk + prefix, bytes + offset, n);
        r = client_parse_response(host->cli, host->id, chunk, prefix + n);
        memset(chunk, 0, sizeof(chunk));
        offset += n;
        if (offset < LOGIN_BLOB_SIZE) assert(!client_is_logged_in(host->cli));
    }
    return r;
}

static Host* active_host;

int request_write(struct Client* cli, ContextID id, struct Buffer* buff) {
    Host* host = active_host;
    assert(host && host->cli == cli);
    assert(buff == &cli->net.send_buffers[id]);
    assert(cli->net.states[id].send_owned && cli->net.states[id].state == CON_RECEIVING);
    assert(buff->size <= sizeof(host->request));
    host->id = id;
    host->size = buff->size;
    host->calls++;
    memcpy(host->request, buff->b, buff->size);
    Request request;
    assert(parse_request(&request, buff->b, buff->size) == 0);
    assert(request.kind == REQUEST_ACCOUNT);
    assert(strcmp(request.data.username, "alice") == 0);
    if (host->retain) {
        assert(!host->pending);
        host->pending = buff;
    } else client_return_buffer(cli, buff);
    return CLIENT_OK;
}

int request_begin(struct Client* cli, ContextID id) {
    assert(active_host && active_host->cli == cli);
    (void)id;
    return CLIENT_OK;
}

int request_end(struct Client* cli, ContextID id) {
    Host* host = active_host;
    if (host->fail) {
        return CLIENT_ERR;
    }
    if (host->synchronous) {
        uint8_t reply[ACCOUNT_RESPONSE_METADATA_SIZE];
        reply_metadata(reply, blob, &identity.pub);
        host->response = client_parse_response(cli, id, reply, sizeof(reply));
        memset(reply, 0, sizeof(reply));
        if (host->response == CLIENT_OK) {
            host->response = deliver_blob(host, blob, 11);
        }
    }
    return CLIENT_OK;
}

void request_abort(struct Client* cli, ContextID id) {
    assert(active_host && active_host->cli == cli);
    (void)id;
}

static Host new_host(void) {
    Host host = {.cli = init_client()};
    assert(host.cli);
    return host;
}

static void begin(Host* host, const uint8_t* pwd, size_t size) {
    active_host = host;
    assert(client_login(host->cli, "alice", pwd, size, &host->id) == CLIENT_OK);
    assert(host->size == 11 && host->request[1] == 1 && host->request[3] == 1);
    assert(host->request[4] == 0 && host->request[5] == 5);
    assert(memcmp(host->request + 6, "alice", 5) == 0);
    assert(!client_is_logged_in(host->cli));
}

static void resolve(Host* host, const uint8_t* bytes, const Key* account) {
    uint8_t reply[ACCOUNT_RESPONSE_METADATA_SIZE];
    reply_metadata(reply, bytes, account);
    assert(client_parse_response(host->cli, host->id, reply, sizeof(reply)) == CLIENT_OK);
    memset(reply, 0, sizeof(reply));
    assert(host->size == 11 && host->request[3] == 1);
    assert(memcmp(host->request + 6, "alice", 5) == 0);
}

static void assert_locked(struct Client* cli) {
    KeyPair zero = {0};
    assert(!client_is_logged_in(cli) && !cli->login_id);
    assert(memcmp(&cli->keys, &zero, sizeof(zero)) == 0);
    assert(sodium_is_zero((const uint8_t*)&cli->account, sizeof(cli->account)));
}

static void password_stays_local(void) {
    const uint8_t other_password[] = "a completely different unlock password";
    const uint8_t* passwords[] = {password, other_password};
    const size_t sizes[] = {sizeof(password) - 1, sizeof(other_password) - 1};
    const uint8_t expected_request[] = {0, 1, 0, 1, 0, 5, 'a', 'l', 'i', 'c', 'e'};
    for (size_t i = 0; i < 2; i++) {
        Host host = new_host();
        host.retain = true;
        begin(&host, passwords[i], sizes[i]);
        LoginOperation* login = host.cli->net.states[host.id].operation;
        assert(host.pending->b != login->password.b);
        assert(host.size == sizeof(expected_request));
        assert(memcmp(host.request, expected_request, sizeof(expected_request)) == 0);
        client_return_buffer(host.cli, host.pending);
        host.pending = NULL;
        resolve(&host, blob, &identity.pub);
        assert(host.calls == 1 && !host.pending);
        assert(client_cancel_login(host.cli) == CLIENT_OK);
        destroy_client(host.cli);
    }
}

static void success_and_retry(void) {
    Host host = new_host();
    uint8_t public_key[KEY_SIZE] = {0}, supplied[sizeof(password)];
    assert(client_get_public_key(host.cli, public_key) == CLIENT_ERR);
    memcpy(supplied, password, sizeof(password));
    begin(&host, supplied, sizeof(password) - 1);
    memset(supplied, 0, sizeof(supplied));
    assert(client_login(host.cli, "bob", password, sizeof(password) - 1, &host.id) == CLIENT_CONN_BUSY);
    resolve(&host, blob, &identity.pub);
    LoginOperation* login = host.cli->net.states[host.id].operation;
    uint8_t* retained_password = login->password.b;
    size_t retained_capacity = login->password.cap;
    assert(deliver_blob(&host, blob, 7) == CLIENT_PARSE_DONE);
    assert(client_is_logged_in(host.cli));
    assert(!host.cli->login_id && !host.cli->net.states[host.id].operation);
    /* Pool free-list pointers occupy the prefix; the rest of the released buffer is wiped. */
    assert(sodium_is_zero(retained_password + sizeof(void*), retained_capacity - sizeof(void*)));
    assert(client_get_public_key(host.cli, public_key) == CLIENT_OK);
    assert(memcmp(public_key, identity.pub.b, KEY_SIZE) == 0);
    assert(memcmp(&host.cli->keys, &identity, sizeof(identity)) == 0);
    assert(memcmp(&host.cli->account, &account_header, sizeof(account_header)) == 0);
    assert(client_login(host.cli, "bob", password, sizeof(password) - 1, &host.id) == CLIENT_CONN_BUSY);
    destroy_client(host.cli);

    host = new_host();
    const uint8_t wrong[] = "wrong password";
    begin(&host, wrong, sizeof(wrong) - 1);
    resolve(&host, blob, &identity.pub);
    assert(deliver_blob(&host, blob, 19) == CLIENT_ERR);
    assert_locked(host.cli);
    begin(&host, password, sizeof(password) - 1);
    resolve(&host, blob, &identity.pub);
    /* Cached ciphertext is reused, but still decrypted with this attempt's password. */
    assert(deliver_blob(&host, blob, LOGIN_BLOB_SIZE) == CLIENT_PARSE_DONE);
    destroy_client(host.cli);
}

static void failures_and_cleanup(void) {
    Host host = new_host();
    active_host = &host;
    assert(client_login(host.cli, "", password, sizeof(password) - 1, &host.id) == CLIENT_ERR);
    char oversized[ACCOUNT_USERNAME_MAX + 2];
    memset(oversized, 'a', sizeof(oversized) - 1);
    oversized[sizeof(oversized) - 1] = 0;
    assert(client_login(host.cli, oversized, password, sizeof(password) - 1, &host.id) == CLIENT_ERR);
    assert(client_login(host.cli, "alice", password, LOGIN_PASSWORD_MAX + 1, &host.id) == CLIENT_ERR);
    assert(client_login(host.cli, "alice", NULL, 1, &host.id) == CLIENT_ERR);
    host.fail = true;
    assert(client_login(host.cli, "alice", password, sizeof(password) - 1, &host.id) == CLIENT_ERR);
    assert_locked(host.cli);
    host.fail = false;
    begin(&host, password, sizeof(password) - 1);
    assert(client_parse_response(host.cli, host.id, NULL, 0) == CLIENT_ERR);
    assert_locked(host.cli);
    begin(&host, password, sizeof(password) - 1);
    uint8_t reply[ACCOUNT_RESPONSE_METADATA_SIZE + 1];
    reply_metadata(reply, blob, &identity.pub);
    reply[71]++;
    assert(client_parse_response(host.cli, host.id, reply, ACCOUNT_RESPONSE_METADATA_SIZE) == CLIENT_ERR);
    assert_locked(host.cli);
    begin(&host, password, sizeof(password) - 1);
    reply_metadata(reply, blob, &identity.pub);
    assert(client_parse_response(host.cli, host.id, reply, sizeof(reply)) == CLIENT_ERR);
    assert_locked(host.cli);

    begin(&host, password, sizeof(password) - 1);
    reply_metadata(reply, blob, &identity.pub);
    assert(client_parse_response(host.cli, host.id, reply, ACCOUNT_RESPONSE_METADATA_SIZE) == CLIENT_OK);
    // A repeated metadata reply cannot be mistaken for header payload.
    assert(client_parse_response(host.cli, host.id, reply, ACCOUNT_RESPONSE_METADATA_SIZE) == CLIENT_ERR);
    assert_locked(host.cli);
    begin(&host, password, sizeof(password) - 1);
    ContextID unrelated = client_new_context(&host.cli->net);
    reply_metadata(reply, blob, &identity.pub);
    assert(client_parse_response(host.cli, unrelated, reply, ACCOUNT_RESPONSE_METADATA_SIZE) == CLIENT_ERR);
    assert(host.cli->login_id == host.id);
    client_free_context(host.cli, unrelated);
    resolve(&host, blob, &identity.pub);
    uint8_t first[HASH_SIZE + sizeof(uint64_t) + 1];
    HashT hash = blob_hash(blob);
    uint64_t size = LOGIN_BLOB_SIZE;
    memcpy(first, hash.b, HASH_SIZE);
    memcpy(first + HASH_SIZE, &size, sizeof(size));
    first[sizeof(first) - 1] = blob[0];
    assert(client_parse_response(host.cli, host.id, first, sizeof(first)) == CLIENT_OK);
    assert(client_cancel_login(host.cli) == CLIENT_OK);
    assert_locked(host.cli);

    begin(&host, password, sizeof(password) - 1);
    resolve(&host, blob, &identity.pub);
    uint8_t corrupted[HASH_SIZE + sizeof(uint64_t) + LOGIN_BLOB_SIZE];
    memcpy(corrupted, hash.b, HASH_SIZE);
    size = LOGIN_BLOB_SIZE + 1;
    memcpy(corrupted + HASH_SIZE, &size, sizeof(size));
    memcpy(corrupted + HASH_SIZE + sizeof(size), blob, LOGIN_BLOB_SIZE);
    assert(client_parse_response(host.cli, host.id, corrupted, sizeof(corrupted)) == CLIENT_ERR);
    assert_locked(host.cli);
    begin(&host, password, sizeof(password) - 1);
    resolve(&host, blob, &identity.pub);
    size = LOGIN_BLOB_SIZE;
    memcpy(corrupted + HASH_SIZE, &size, sizeof(size));
    corrupted[sizeof(corrupted) - 1] ^= 1;
    assert(client_parse_response(host.cli, host.id, corrupted, sizeof(corrupted)) == CLIENT_ERR);
    assert_locked(host.cli);
    assert(!store_get_item(&host.cli->blob_store, &hash));
    begin(&host, password, sizeof(password) - 1);
    resolve(&host, blob, &identity.pub);
    uint8_t oversized_chunk[sizeof(corrupted) + 1] = {0};
    memcpy(oversized_chunk, hash.b, HASH_SIZE);
    memcpy(oversized_chunk + HASH_SIZE, &size, sizeof(size));
    assert(client_parse_response(host.cli, host.id, oversized_chunk, sizeof(oversized_chunk)) == CLIENT_ERR);
    assert_locked(host.cli);
    assert(!store_get_item(&host.cli->blob_store, &hash));
    begin(&host, password, sizeof(password) - 1);
    resolve(&host, blob, &identity.pub);
    first[0] ^= 1;
    assert(client_parse_response(host.cli, host.id, first, sizeof(first)) == CLIENT_ERR);
    assert_locked(host.cli);

    begin(&host, password, sizeof(password) - 1);
    Key wrong_account = {{0}};
    resolve(&host, blob, &wrong_account);
    assert(deliver_blob(&host, blob, 17) == CLIENT_ERR);
    assert_locked(host.cli);
    begin(&host, password, sizeof(password) - 1);
    destroy_client(host.cli); /* Pending password is wiped during shutdown. */
}

static void exhausted_resources(void) {
    Host host = new_host();
    active_host = &host;
    for (size_t i = 1; i < MAX_CONNS; i++) assert(valid_id(client_new_context(&host.cli->net)));
    assert(client_login(host.cli, "alice", password, sizeof(password) - 1, &host.id) == CLIENT_CONN_BUSY);
    assert_locked(host.cli);
    destroy_client(host.cli);

    host = new_host();
    active_host = &host;
    buffer_pool_destroy(&host.cli->pool);
    assert(buffer_pool_init(&host.cli->pool, 256, 1, 4096, 1, 65536, 1) == 0);
    size_t caps[] = {256, 4096, 65536};
    uint8_t* held[3];
    for (size_t i = 0; i < 3; i++) {
        held[i] = buffer_pool_pop(&host.cli->pool, &caps[i]);
        assert(held[i]);
    }
    assert(client_login(host.cli, "alice", password, sizeof(password) - 1, &host.id) == CLIENT_ERR);
    assert_locked(host.cli);
    assert(host.cli->net.free_head == 1);
    for (size_t i = 0; i < 3; i++) assert(buffer_pool_push(&host.cli->pool, held[i], caps[i]) == 0);
    for (size_t i = 1; i < 3; i++) {
        held[i] = buffer_pool_pop(&host.cli->pool, &caps[i]);
        assert(held[i]);
    }
    assert(client_login(host.cli, "alice", password, sizeof(password) - 1, &host.id) == CLIENT_ERR);
    assert_locked(host.cli);
    assert(host.cli->pool.buckets[0].available == 1 && !host.calls);
    for (size_t i = 1; i < 3; i++) assert(buffer_pool_push(&host.cli->pool, held[i], caps[i]) == 0);
    begin(&host, password, sizeof(password) - 1);
    resolve(&host, blob, &identity.pub);
    assert(deliver_blob(&host, blob, 9) == CLIENT_PARSE_DONE);
    destroy_client(host.cli);
}

static void retained_requests(void) {
    Host host = new_host();
    host.retain = true;
    begin(&host, password, sizeof(password) - 1);
    ContextID first = host.id;
    Buffer* request = host.pending;
    assert(context_release_send_buffer(host.cli, first) == CLIENT_CONN_BUSY);
    uint8_t reply[ACCOUNT_RESPONSE_METADATA_SIZE];
    reply_metadata(reply, blob, &identity.pub);
    assert(client_parse_response(host.cli, first, reply, sizeof(reply)) == CLIENT_OK);
    memset(reply, 0, sizeof(reply));
    LoginOperation* login = host.cli->net.states[host.id].operation;
    assert(host.calls == 1 && login->metadata_ready);
    assert(memcmp(request->b + 6, "alice", 5) == 0);
    ContextID retry;
    assert(client_login(
        host.cli, "alice", password, sizeof(password) - 1, &retry
    ) == CLIENT_CONN_BUSY);

    // All replies can arrive while the runtime still holds the outgoing buffer.
    assert(deliver_blob(&host, blob, 5) == CLIENT_PARSE_DONE);
    assert(host.calls == 1 && host.cli->net.states[first].release_pending);
    assert(memcmp(request->b + 6, "alice", 5) == 0);
    assert(host.cli->net.free_head != first);
    client_return_buffer(host.cli, request);
    assert(host.calls == 1 && host.cli->net.free_head == first);
    host.pending = NULL;
    destroy_client(host.cli);

    host = new_host();
    host.retain = true;
    begin(&host, password, sizeof(password) - 1);
    first = host.id;
    request = host.pending;
    assert(client_cancel_login(host.cli) == CLIENT_OK);
    assert_locked(host.cli);
    assert(memcmp(request->b + 6, "alice", 5) == 0);
    assert(host.cli->net.free_head != first);
    // A new login cannot reuse a context whose request is still held by the host.
    host.pending = NULL;
    begin(&host, password, sizeof(password) - 1);
    assert(host.id != first);
    client_return_buffer(host.cli, request);
    assert(host.cli->login_id == host.id);
    assert(client_cancel_login(host.cli) == CLIENT_OK);
    client_return_buffer(host.cli, host.pending);
    destroy_client(host.cli);
}

static void authenticated_bad_record(uint8_t tag, uint8_t kind, bool invalid_pages) {
    uint8_t changed[LOGIN_BLOB_SIZE], key[32], plain[ACCOUNT_HEADER_SIZE] = {0}, ad[71];
    memcpy(changed, blob, sizeof(changed));
    assert(crypto_pwhash(key, sizeof(key), (const char*)password, sizeof(password) - 1,
                        changed + 20, 2, 64 * 1024 * 1024, crypto_pwhash_ALG_ARGON2ID13) == 0);
    size_t size;
    assert(marshal_account_header(plain, sizeof(plain), &size, &account_header) == 0);
    plain[3] = kind;
    if (invalid_pages) plain[11] = 8; // First feed page exceeds current page 7.
    crypto_secretstream_xchacha20poly1305_state stream;
    assert(crypto_secretstream_xchacha20poly1305_init_push(&stream, changed + 36, key) == 0);
    memcpy(ad, changed, 64);
    ad[64] = 0;
    ad[65] = 5;
    memcpy(ad + 66, "alice", 5);
    assert(crypto_secretstream_xchacha20poly1305_push(&stream, changed + 64, NULL,
                        plain, sizeof(plain), ad, sizeof(ad), tag) == 0);
    KeyPair recovered = {0};
    AccountHeader data = {0};
    assert(decrypt_account_header(&data, &recovered, &identity.pub, "alice", password,
                        sizeof(password) - 1, changed, sizeof(changed)) == -1);
    assert(sodium_is_zero((const uint8_t*)&recovered, sizeof(recovered)));
    assert(sodium_is_zero((const uint8_t*)&data, sizeof(data)));
    sodium_memzero(plain, sizeof(plain));
    sodium_memzero(key, sizeof(key));
    sodium_memzero(&stream, sizeof(stream));
}

static void authenticated_record_checks(void) {
    KeyPair recovered = {0};
    AccountHeader data = {0};
    uint8_t changed[LOGIN_BLOB_SIZE];
    assert(decrypt_account_header(&data, &recovered, &identity.pub, "bob", password,
                        sizeof(password) - 1, blob, sizeof(blob)) == -1);
    const size_t offsets[] = {0, 2, 4, 8, 12, 20, 36, 60, 64, LOGIN_BLOB_SIZE - 1};
    for (size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++) {
        size_t offset = offsets[i];
        memcpy(changed, blob, sizeof(blob));
        changed[offset] ^= 1;
        assert(decrypt_account_header(&data, &recovered, &identity.pub, "alice", password,
                            sizeof(password) - 1, changed, sizeof(changed)) == -1);
    }
    assert(sodium_is_zero((const uint8_t*)&recovered, sizeof(recovered)) && sodium_is_zero((const uint8_t*)&data, sizeof(data)));
    for (size_t size = 0; size < sizeof(blob); size++)
        assert(decrypt_account_header(&data, &recovered, &identity.pub, "alice", password,
                            sizeof(password) - 1, blob, size) == -1);

    const uint8_t replacement[] = "replacement password";
    assert(encrypt_account_header(changed, "alice", replacement, sizeof(replacement) - 1, &account_header) == 0);
    assert(memcmp(changed + 20, blob + 20, 16) != 0);
    assert(memcmp(changed + 36, blob + 36, 24) != 0);
    assert(decrypt_account_header(&data, &recovered, &identity.pub, "alice", password,
                        sizeof(password) - 1, changed, sizeof(changed)) == -1);
    assert(decrypt_account_header(&data, &recovered, &identity.pub, "alice", replacement,
                        sizeof(replacement) - 1, changed, sizeof(changed)) == 0);
    assert(memcmp(&recovered, &identity, sizeof(identity)) == 0);
    assert(memcmp(&data, &account_header, sizeof(data)) == 0);
    sodium_memzero(&recovered, sizeof(recovered));
    sodium_memzero(&data, sizeof(data));
}

static void synchronous_transport(void) {
    Host host = new_host();
    host.synchronous = true;
    active_host = &host;
    assert(client_login(host.cli, "alice", password, sizeof(password) - 1, &host.id) == CLIENT_PARSE_DONE);
    assert(client_is_logged_in(host.cli) && host.calls == 1);
    destroy_client(host.cli);
    host = new_host();
    host.synchronous = true;
    active_host = &host;
    const uint8_t wrong[] = "wrong password";
    assert(client_login(host.cli, "alice", wrong, sizeof(wrong) - 1, &host.id) == CLIENT_ERR);
    assert_locked(host.cli);
    assert(host.response == CLIENT_ERR);
    destroy_client(host.cli);
}

static void failed_header_outputs(void) {
    AccountHeader header;
    KeyPair keys;
    memset(&header, 0xa5, sizeof(header));
    memset(&keys, 0xa5, sizeof(keys));
    AccountHeader before = header;
    KeyPair keys_before = keys;

    const uint8_t wrong[] = "wrong password";
    assert(decrypt_account_header(&header, &keys, &identity.pub, "alice", wrong,
        sizeof(wrong) - 1, blob, sizeof(blob)) == -1);
    assert(memcmp(&header, &before, sizeof(header)) == 0);
    assert(memcmp(&keys, &keys_before, sizeof(keys)) == 0);

    Key another_account = identity.pub;
    another_account.b[0] ^= 1;
    assert(decrypt_account_header(&header, &keys, &another_account, "alice", password,
        sizeof(password) - 1, blob, sizeof(blob)) == -1);
    assert(memcmp(&header, &before, sizeof(header)) == 0);
    assert(memcmp(&keys, &keys_before, sizeof(keys)) == 0);

    uint8_t out[LOGIN_BLOB_SIZE];
    memset(out, 0xa5, sizeof(out));
    header = account_header;
    header.first_feed_page = header.current_feed_page + 1;
    assert(encrypt_account_header(out, "alice", password, sizeof(password) - 1, &header) == -1);
    for (size_t i = 0; i < sizeof(out); i++) {
        assert(out[i] == 0xa5);
    }

    assert(encrypt_account_header(out, "alice", password, sizeof(password) - 1, NULL) == -1);
    assert(decrypt_account_header(NULL, &keys, &identity.pub, "alice", password,
        sizeof(password) - 1, blob, sizeof(blob)) == -1);
    assert(decrypt_account_header(&header, NULL, &identity.pub, "alice", password,
        sizeof(password) - 1, blob, sizeof(blob)) == -1);
}

int main(void) {
    assert(sodium_init() >= 0);
    uint8_t seed[32] = {1, 2, 3};
    assert(crypto_sign_seed_keypair(identity.pub.b, identity.priv.b, seed) == 0);
    memset(owner_key.b, 0xa5, KEY_SIZE);
    account_header.first_feed_page = 2;
    account_header.current_feed_page = 7;
    account_header.first_recipient_page = 1;
    account_header.current_recipient_page = 3;
    memset(account_header.inbox_head_locator.b, 0xcc, HASH_SIZE);
    memcpy(account_header.signing_seed.b, seed, sizeof(seed));
    account_header.data_key = owner_key;
    assert(encrypt_account_header(blob, "alice", password, sizeof(password) - 1, &account_header) == 0);
    password_stays_local();
    success_and_retry();
    failures_and_cleanup();
    exhausted_resources();
    authenticated_record_checks();
    failed_header_outputs();
    authenticated_bad_record(crypto_secretstream_xchacha20poly1305_TAG_MESSAGE, 2, false);
    authenticated_bad_record(crypto_secretstream_xchacha20poly1305_TAG_FINAL, 99, false);
    authenticated_bad_record(crypto_secretstream_xchacha20poly1305_TAG_FINAL, 2, true);
    synchronous_transport();
    retained_requests();
    sodium_memzero(&identity, sizeof(identity));
    sodium_memzero(&owner_key, sizeof(owner_key));
    sodium_memzero(&account_header, sizeof(account_header));
    return 0;
}
