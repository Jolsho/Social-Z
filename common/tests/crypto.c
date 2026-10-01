/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/crypto.h"
#include <sodium.h>
#include <assert.h>

static void signing(void) {
    KeyPair alice, bob;
    assert(new_keypair(&alice) == 0);
    assert(new_keypair(&bob) == 0);
    HashT hash = {{1, 2, 3}};
    Signature sig;
    assert(sign_hash(&alice.priv, &sig, &hash) == 0);
    assert(valid_signature(&alice.pub, &sig, &hash));
    assert(!valid_signature(&bob.pub, &sig, &hash));
    hash.b[31] ^= 1;
    assert(!valid_signature(&alice.pub, &sig, &hash));
    hash.b[31] ^= 1;
    sig.b[63] ^= 1;
    assert(!valid_signature(&alice.pub, &sig, &hash));
    sodium_memzero(&alice, sizeof(alice));
    sodium_memzero(&bob, sizeof(bob));
}

static void exchange(void) {
    KeyPair alice, bob;
    Key alice_rx, alice_tx, bob_rx, bob_tx;
    assert(new_keypair(&alice) == 0);
    assert(new_keypair(&bob) == 0);
    assert(auth_session_keys(&alice_rx, &alice_tx, &alice, &bob.pub, false) == 0);
    assert(auth_session_keys(&bob_rx, &bob_tx, &bob, &alice.pub, true) == 0);
    assert(memcmp(alice_rx.b, bob_tx.b, KEY_SIZE) == 0);
    assert(memcmp(alice_tx.b, bob_rx.b, KEY_SIZE) == 0);
    assert(memcmp(alice_rx.b, alice_tx.b, KEY_SIZE) != 0);

    /* Initial keys actually encrypt in both directions. */
    const unsigned char message[] = "temporary exchange key";
    unsigned char encrypted[sizeof(message) + crypto_aead_chacha20poly1305_ABYTES];
    unsigned char decrypted[sizeof(message)];
    Nonce nonce = {{0}};
    unsigned long long length;
    assert(crypto_aead_chacha20poly1305_encrypt(encrypted, &length, message, sizeof(message),
           NULL, 0, NULL, nonce.b, alice_tx.b) == 0);
    assert(crypto_aead_chacha20poly1305_decrypt(decrypted, &length, NULL, encrypted, sizeof(encrypted),
           NULL, 0, nonce.b, bob_rx.b) == 0);
    assert(length == sizeof(message) && memcmp(message, decrypted, length) == 0);
    assert(crypto_aead_chacha20poly1305_encrypt(encrypted, &length, message, sizeof(message),
           NULL, 0, NULL, nonce.b, bob_tx.b) == 0);
    assert(crypto_aead_chacha20poly1305_decrypt(decrypted, &length, NULL, encrypted, sizeof(encrypted),
           NULL, 0, nonce.b, alice_rx.b) == 0);
    assert(length == sizeof(message) && memcmp(message, decrypted, length) == 0);

    ExchangeKeyPair a, b;
    assert(crypto_kx_keypair(a.pub.b, a.priv.b) == 0);
    assert(crypto_kx_keypair(b.pub.b, b.priv.b) == 0);
    assert(crypto_kx_client_session_keys(alice_rx.b, alice_tx.b, a.pub.b, a.priv.b, b.pub.b) == 0);
    assert(crypto_kx_server_session_keys(bob_rx.b, bob_tx.b, b.pub.b, b.priv.b, a.pub.b) == 0);
    assert(memcmp(alice_rx.b, bob_tx.b, KEY_SIZE) == 0);
    assert(memcmp(alice_tx.b, bob_rx.b, KEY_SIZE) == 0);

    Key zero = {{0}};
    assert(auth_session_keys(&alice_rx, &alice_tx, &alice, &zero, false) == -1);
    assert(key_is_zero(&alice_rx) && key_is_zero(&alice_tx));
    alice.pub = zero;
    memset(&alice_rx, 1, sizeof(alice_rx));
    memset(&alice_tx, 1, sizeof(alice_tx));
    assert(auth_session_keys(&alice_rx, &alice_tx, &alice, &bob.pub, true) == -1);
    assert(key_is_zero(&alice_rx) && key_is_zero(&alice_tx));
    sodium_memzero(&alice, sizeof(alice));
    sodium_memzero(&bob, sizeof(bob));
    sodium_memzero(&a, sizeof(a));
    sodium_memzero(&b, sizeof(b));
}

int main(void) {
    signing();
    exchange();
    return 0;
}
