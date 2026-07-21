/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz/codec.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

bool valid_signature(Key* signer, Signature* sig, HashT* hash);
int sign_hash(Key* signer, Signature* sig, HashT* hash);

int new_keypair(KeyPair* keys);

size_t encoded_key_len();
void key_to_str(char* key_str, Key* key);
int str_to_key(const char* key_str, Key* key);
static inline bool key_is_zero(const Key* k) {
    for (int i = 0; i < KEY_SIZE; i++) {
        if (k->b[i] != 0) return false;
    }
    return true;
}

size_t encoded_hash_len();
void hash_to_str(char* h_str, HashT* h);
int str_to_hash(const char* h_str, HashT* h);

#ifdef __cplusplus
}
#endif
