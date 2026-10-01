/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/login.h"
#include <sodium.h>

enum { HEADER_SIZE = 64, PLAIN_SIZE = 68, CIPHER_SIZE = 85 };
_Static_assert(CIPHER_SIZE == PLAIN_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES, "Login ciphertext size");
_Static_assert(crypto_pwhash_SALTBYTES == 16, "Login salt size");
_Static_assert(crypto_secretstream_xchacha20poly1305_HEADERBYTES == 24, "Login stream header size");

static uint64_t read_uint(const uint8_t* p, size_t n) {
    uint64_t v = 0;

    for (size_t i = 0; i < n; i++)
        v = (v << 8) | p[i];

    return v;
}

static void write_uint(
    uint8_t* p,
    uint64_t v,
    size_t n
) {
    while (n) {
        p[--n] = (uint8_t)v;
        v >>= 8;
    }
}

size_t login_username_size(const char* username) {
    if (!username)
        return 0;

    size_t n = 0;
    while (n <= LOGIN_USERNAME_MAX && username[n])
        n++;

    return n && n <= LOGIN_USERNAME_MAX ? n : 0;
}

size_t login_lookup_request(uint8_t out[6 + LOGIN_USERNAME_MAX], const char* username) {
    size_t n = login_username_size(username);
    if (!out || !n)
        return 0;

    write_uint(out, 1, 2);
    write_uint(out + 2, 1, 2);
    write_uint(out + 4, n, 2);
    memcpy(out + 6, username, n);

    return 6 + n;
}

void login_fetch_request(uint8_t out[LOGIN_FETCH_SIZE], const HashT* hash) {
    write_uint(out, 1, 2);
    write_uint(out + 2, 2, 2);
    memcpy(out + 4, hash->b, HASH_SIZE);
}

int login_read_lookup(
    LoginLookup* out,
    const uint8_t* bytes,
    size_t size
) {
    if (!out || !bytes || size != LOGIN_LOOKUP_SIZE ||
        read_uint(bytes, 2) != 1 || read_uint(bytes + 2, 2) != 1 ||
        read_uint(bytes + 68, 4) != LOGIN_BLOB_SIZE)
        return -1;

    memcpy(out->account.b, bytes + 4, KEY_SIZE);
    memcpy(out->hash.b, bytes + 36, HASH_SIZE);

    out->size = LOGIN_BLOB_SIZE;

    return 0;
}

static size_t associated_data(
    uint8_t* out,
    const uint8_t* header,
    const char* username
) {
    size_t n = login_username_size(username);
    memcpy(out, header, HEADER_SIZE);
    write_uint(out + HEADER_SIZE, n, 2);
    memcpy(out + HEADER_SIZE + 2, username, n);

    return HEADER_SIZE + 2 + n;
}

static int password_key(
    uint8_t* key,
    const uint8_t* password,
    size_t n,
    const uint8_t* header
) {
    /* V1 accepts one bounded profile, independent of changing library defaults. */
    if (!password || !n || n > LOGIN_PASSWORD_MAX ||
        read_uint(header, 2) != 1 || read_uint(header + 2, 2) != 1 ||
        read_uint(header + 4, 4) != crypto_pwhash_ALG_ARGON2ID13 ||
        read_uint(header + 8, 4) != 2 || read_uint(header + 12, 8) != 64 * 1024 * 1024 ||
        read_uint(header + 60, 4) != CIPHER_SIZE)
        return -1;

    return crypto_pwhash(key, KEY_SIZE, (const char*)password, n, header + 20,
                        2, 64 * 1024 * 1024, crypto_pwhash_ALG_ARGON2ID13);
}

int login_encrypt(
    uint8_t out[LOGIN_BLOB_SIZE],
    const char* username,
    const uint8_t* password,
    size_t password_size,
    const KeyPair* keys,
    const Key* data_key
) {
    if (!out || !keys || !data_key || !login_username_size(username) ||
        !password || !password_size || password_size > LOGIN_PASSWORD_MAX || sodium_init() < 0)
        return -1;

    uint8_t blob[LOGIN_BLOB_SIZE] = {0},
            plain[PLAIN_SIZE] = {0},
            key[KEY_SIZE] = {0};
    uint8_t ad[HEADER_SIZE + 2 + LOGIN_USERNAME_MAX];
    crypto_secretstream_xchacha20poly1305_state stream = {0};
    int r = -1;

    write_uint(blob, 1, 2);
    write_uint(blob + 2, 1, 2);
    write_uint(blob + 4, crypto_pwhash_ALG_ARGON2ID13, 4);
    write_uint(blob + 8, 2, 4);
    write_uint(blob + 12, 64 * 1024 * 1024, 8);
    randombytes_buf(blob + 20, 16);
    write_uint(blob + 60, CIPHER_SIZE, 4);

    write_uint(plain, 1, 2);
    write_uint(plain + 2, 13, 2);
    crypto_sign_ed25519_sk_to_seed(plain + 4, keys->priv.b);
    memcpy(plain + 36, data_key->b, KEY_SIZE);

    if (password_key(key, password, password_size, blob) == 0 &&
        crypto_secretstream_xchacha20poly1305_init_push(&stream, blob + 36, key) == 0) {
        size_t ad_size = associated_data(ad, blob, username);
        if (crypto_secretstream_xchacha20poly1305_push(&stream, blob + HEADER_SIZE, NULL,
            plain, sizeof(plain), ad, ad_size, crypto_secretstream_xchacha20poly1305_TAG_FINAL) == 0) {
            memcpy(out, blob, sizeof(blob));
            r = 0;
        }
    }

    sodium_memzero(key, sizeof(key));
    sodium_memzero(plain, sizeof(plain));
    sodium_memzero(&stream, sizeof(stream));

    return r;
}

int login_decrypt(
    KeyPair* keys,
    Key* data_key,
    const Key* account,
    const char* username,
    const uint8_t* password,
    size_t password_size,
    const uint8_t* blob,
    size_t blob_size
) {
    if (!keys || !data_key || !account || !blob || blob_size != LOGIN_BLOB_SIZE ||
        !login_username_size(username) || sodium_init() < 0)
        return -1;

    uint8_t key[KEY_SIZE] = {0},
            plain[PLAIN_SIZE] = {0},
            tag = 0;
    uint8_t ad[HEADER_SIZE + 2 + LOGIN_USERNAME_MAX];
    KeyPair recovered = {0};
    Key converted;
    unsigned long long size = 0;
    crypto_secretstream_xchacha20poly1305_state stream = {0};
    size_t ad_size = associated_data(ad, blob, username);
    int r = -1;

    if (password_key(key, password, password_size, blob) == 0 &&
        crypto_secretstream_xchacha20poly1305_init_pull(&stream, blob + 36, key) == 0 &&
        crypto_secretstream_xchacha20poly1305_pull(&stream, plain, &size, &tag,
            blob + HEADER_SIZE, CIPHER_SIZE, ad, ad_size) == 0 &&
        size == PLAIN_SIZE && tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL &&
        read_uint(plain, 2) == 1 && read_uint(plain + 2, 2) == 13 &&
        crypto_sign_seed_keypair(recovered.pub.b, recovered.priv.b, plain + 4) == 0 &&
        sodium_memcmp(recovered.pub.b, account->b, KEY_SIZE) == 0 &&
        crypto_sign_ed25519_pk_to_curve25519(converted.b, recovered.pub.b) == 0) {
        *keys = recovered;
        memcpy(data_key->b, plain + 36, KEY_SIZE);
        r = 0;
    }

    sodium_memzero(key, sizeof(key));
    sodium_memzero(plain, sizeof(plain));
    sodium_memzero(&recovered, sizeof(recovered));
    sodium_memzero(&stream, sizeof(stream));

    return r;
}
