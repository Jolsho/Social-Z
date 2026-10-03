/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "codec/account.h"
#include "utils/crypto.h"
#include <stdbool.h>
#include <sodium.h>

// Password envelope: public KDF profile, salt, stream header, and ciphertext length.
enum { 
    HEADER_SIZE = 64, 
    PLAIN_SIZE = ACCOUNT_HEADER_SIZE, 
    CIPHER_SIZE = PLAIN_SIZE + 17 
};
_Static_assert(CIPHER_SIZE == PLAIN_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES, "Account ciphertext size");
_Static_assert(LOGIN_BLOB_SIZE == HEADER_SIZE + CIPHER_SIZE, "Account blob size");
_Static_assert(crypto_pwhash_SALTBYTES == 16, "Account salt size");
_Static_assert(crypto_secretstream_xchacha20poly1305_HEADERBYTES == 24, "Account stream header size");

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

static size_t associated_data(
    uint8_t* out,
    const uint8_t* header,
    const char* username
) {
    size_t n = account_username_size(username);
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

static bool valid_pages(const AccountHeader* header) {
    return header->first_feed_page <= header->current_feed_page &&
           header->first_recipient_page <= header->current_recipient_page;
}

int marshal_account_header(
    uint8_t* out,
    size_t capacity,
    size_t* size,
    const AccountHeader* header
) {
    if (!out || !size || !header || capacity < ACCOUNT_HEADER_SIZE) {
        return -1;
    }

    if (!valid_pages(header)) {
        return -1;
    }

    // Big-endian u16 fields: version 1, then record kind 2 (account header).
    out[0] = 0;
    out[1] = 1;
    out[2] = 0;
    out[3] = 2;

    write_uint(out + 4, header->first_feed_page, 8);
    write_uint(out + 12, header->current_feed_page, 8);

    write_uint(out + 20, header->first_recipient_page, 8);
    write_uint(out + 28, header->current_recipient_page, 8);

    memcpy(out + 36, header->inbox_head_locator.b, HASH_SIZE);

    memcpy(out + 68, header->signing_seed.b, KEY_SIZE);
    memcpy(out + 100, header->data_key.b, KEY_SIZE);

    *size = ACCOUNT_HEADER_SIZE;
    return 0;
}

int parse_account_header(
    AccountHeader* out,
    const uint8_t* bytes,
    size_t size
) {
    if (!out || !bytes || size != ACCOUNT_HEADER_SIZE) {
        return -1;
    }

    if (bytes[0] != 0 || bytes[1] != 1 || bytes[2] != 0 || bytes[3] != 2) {
        return -1;
    }

    AccountHeader header = {0};

    header.first_feed_page = read_uint(bytes + 4, 8);
    header.current_feed_page = read_uint(bytes + 12, 8);

    header.first_recipient_page = read_uint(bytes + 20, 8);
    header.current_recipient_page = read_uint(bytes + 28, 8);

    memcpy(header.inbox_head_locator.b, bytes + 36, HASH_SIZE);

    if (!valid_pages(&header)) {
        return -1;
    }

    memcpy(header.signing_seed.b, bytes + 68, KEY_SIZE);
    memcpy(header.data_key.b, bytes + 100, KEY_SIZE);

    *out = header;
    return 0;
}

int encrypt_account_header(
    uint8_t out[LOGIN_BLOB_SIZE],
    const char* username,
    const uint8_t* password,
    size_t password_size,
    const AccountHeader* header
) {
    if (!out || !header || !account_username_size(username) ||
        !password || !password_size || password_size > LOGIN_PASSWORD_MAX || sodium_init() < 0)
        return -1;

    uint8_t blob[LOGIN_BLOB_SIZE] = {0};
    Key key = {0};
    uint8_t ad[HEADER_SIZE + 2 + ACCOUNT_USERNAME_MAX];
    CryptCtx crypto = {0};
    int r = -1;

    write_uint(blob, 1, 2);
    write_uint(blob + 2, 1, 2);
    write_uint(blob + 4, crypto_pwhash_ALG_ARGON2ID13, 4);
    write_uint(blob + 8, 2, 4);
    write_uint(blob + 12, 64 * 1024 * 1024, 8);
    randombytes_buf(blob + 20, 16);
    write_uint(blob + 60, CIPHER_SIZE, 4);

    size_t plain_size;
    if (marshal_account_header(blob + HEADER_SIZE, PLAIN_SIZE, &plain_size, header) != 0)
        goto done;

    if (password_key(key.b, password, password_size, blob) == 0 &&
        crypt_init_encrypt(&crypto, blob + 36, &key) == 0) {
        size_t ad_size = associated_data(ad, blob, username);
        Buffer record = {blob + HEADER_SIZE, CIPHER_SIZE, PLAIN_SIZE};
        if (encrypt(&crypto, &record, ad, ad_size, true) == 0) {
            memcpy(out, blob, sizeof(blob));
            r = 0;
        }
    }

done:
    sodium_memzero(&key, sizeof(key));
    sodium_memzero(blob, sizeof(blob));
    crypt_clear(&crypto);

    return r;
}

int decrypt_account_header(
    AccountHeader* header,
    KeyPair* keys,
    const Key* account,
    const char* username,
    const uint8_t* password,
    size_t password_size,
    const uint8_t* blob,
    size_t blob_size
) {
    if (!header || !keys || !account || !blob || blob_size != LOGIN_BLOB_SIZE ||
        !account_username_size(username) || sodium_init() < 0)
        return -1;

    Key key = {0};
    uint8_t plain[CIPHER_SIZE] = {0};
    uint8_t ad[HEADER_SIZE + 2 + ACCOUNT_USERNAME_MAX];
    KeyPair recovered = {0};
    AccountHeader parsed = {0};
    Key converted;
    CryptCtx crypto = {0};
    size_t ad_size = associated_data(ad, blob, username);
    int r = -1;

    memcpy(plain, blob + HEADER_SIZE, sizeof(plain));
    Buffer record = {plain, sizeof(plain), sizeof(plain)};

    if (password_key(key.b, password, password_size, blob) == 0 &&
        crypt_init_decrypt(&crypto, blob + 36, &key) == 0 &&
        decrypt(&crypto, &record, ad, ad_size, true) == 0 &&
        parse_account_header(&parsed, plain, record.size) == 0 &&
        crypto_sign_seed_keypair(recovered.pub.b, recovered.priv.b, parsed.signing_seed.b) == 0 &&
        sodium_memcmp(recovered.pub.b, account->b, KEY_SIZE) == 0 &&
        crypto_sign_ed25519_pk_to_curve25519(converted.b, recovered.pub.b) == 0) {
        *keys = recovered;
        *header = parsed;
        r = 0;
    }

    sodium_memzero(&key, sizeof(key));
    sodium_memzero(plain, sizeof(plain));
    sodium_memzero(&recovered, sizeof(recovered));
    sodium_memzero(&parsed, sizeof(parsed));
    crypt_clear(&crypto);

    return r;
}
