/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/crypto.h"
#include "sz_common/codec.h"
#include <sodium/crypto_sign.h>
#include <sodium/core.h>
#include <sodium/crypto_kx.h>
#include <sodium/utils.h>
#include <sodium/crypto_aead_chacha20poly1305.h>
#include <sodium/crypto_secretstream_xchacha20poly1305.h>

_Static_assert(SIGNING_KEY_SIZE == crypto_sign_SECRETKEYBYTES, "Signing key size");
_Static_assert(KEY_SIZE == crypto_sign_PUBLICKEYBYTES, "Signing public key size");
_Static_assert(KEY_SIZE == crypto_kx_SECRETKEYBYTES, "Exchange private key size");
_Static_assert(KEY_SIZE == crypto_kx_PUBLICKEYBYTES, "Exchange public key size");
_Static_assert(KEY_SIZE == crypto_kx_SESSIONKEYBYTES, "Session key size");
_Static_assert(SIGNATURE_SIZE == crypto_sign_BYTES, "Signature size");

bool valid_signature(Key* signer, Signature* sig, HashT* hash) {
    return crypto_sign_verify_detached(sig->b, hash->b, HASH_SIZE, signer->b) == 0;
}
int sign_hash(const SigningKey* signer, Signature* sig, const HashT* hash) {
    unsigned long long sig_len = SIGNATURE_SIZE;
    return crypto_sign_detached(sig->b, &sig_len, hash->b, HASH_SIZE, signer->b);
}

int new_keypair(KeyPair* keys) {
    if (sodium_init() < 0) return -1;
    return crypto_sign_keypair(keys->pub.b, keys->priv.b);
}

int auth_session_keys(Key* rx, Key* tx, const KeyPair* local, const Key* remote, bool inbound) {
    ExchangeKeyPair converted = {0};
    Key peer;
    int r = -1;
    if (crypto_sign_ed25519_pk_to_curve25519(converted.pub.b, local->pub.b) == 0 &&
        crypto_sign_ed25519_sk_to_curve25519(converted.priv.b, local->priv.b) == 0 &&
        crypto_sign_ed25519_pk_to_curve25519(peer.b, remote->b) == 0) {
        r = inbound ? crypto_kx_server_session_keys(rx->b, tx->b, converted.pub.b, converted.priv.b, peer.b)
                    : crypto_kx_client_session_keys(rx->b, tx->b, converted.pub.b, converted.priv.b, peer.b);
    }
    sodium_memzero(&converted, sizeof(converted));
    if (r != 0) {
        sodium_memzero(rx, sizeof(*rx));
        sodium_memzero(tx, sizeof(*tx));
    }
    return r;
}

size_t encoded_key_len() {
    return sodium_base64_encoded_len(KEY_SIZE, sodium_base64_VARIANT_ORIGINAL);
}

void key_to_str(char* key_str, Key* key) {
    sodium_bin2base64(
        key_str,
        encoded_key_len(),
        key->b,
        KEY_SIZE,
        sodium_base64_VARIANT_ORIGINAL
    );
}

int str_to_key(const char* key_str, Key* key) {
    size_t b64_len = sodium_base64_encoded_len(KEY_SIZE, sodium_base64_VARIANT_ORIGINAL);
    size_t actual_len = 0;
    int r = sodium_base642bin(
        key->b,          // output buffer
        KEY_SIZE,       // max output length
        key_str,        // input string
        b64_len,        // length of input string
        NULL,        // ignore chars
        &actual_len,    // actual bytes written
        NULL,        // end pointer (optional)
        sodium_base64_VARIANT_ORIGINAL
    );
    if (r < 0) return r;
    return b64_len;
}

size_t encoded_hash_len() {
    return sodium_base64_encoded_len(HASH_SIZE, sodium_base64_VARIANT_ORIGINAL);
}

void hash_to_str(char* h_str, HashT* h) {
    sodium_bin2base64(
        h_str,
        encoded_hash_len(),
        h->b,
        KEY_SIZE,
        sodium_base64_VARIANT_ORIGINAL
    );
}

int str_to_hash(const char* h_str, HashT* h) {
    size_t b64_len = sodium_base64_encoded_len(HASH_SIZE, sodium_base64_VARIANT_ORIGINAL);
    size_t actual_len = 0;
    int r = sodium_base642bin(
        h->b,          // output buffer
        HASH_SIZE,       // max output length
        h_str,        // input string
        b64_len,        // length of input string
        NULL,        // ignore chars
        &actual_len,    // actual bytes written
        NULL,        // end pointer (optional)
        sodium_base64_VARIANT_ORIGINAL
    );
    if (r < 0) return r;
    return b64_len;
}
