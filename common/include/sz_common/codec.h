/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t ConnID;
typedef int SSID;
typedef int TrxID;
typedef int16_t PktCode;

#define HASH_SIZE 32
typedef struct HashT {
    unsigned char b[HASH_SIZE];
} HashT;

#define KEY_SIZE 32
typedef struct Key {
    unsigned char b[KEY_SIZE];
} Key;

typedef struct KeyPair {
    Key priv;
    Key pub;
} KeyPair;

#define SIGNATURE_SIZE 64
typedef struct Signature {
    unsigned char b[SIGNATURE_SIZE];
} Signature;

#define NONCE_SIZE 8
typedef struct Nonce {
    unsigned char b[NONCE_SIZE];
} Nonce;

typedef struct Card {
    int     id;
    HashT   hash;
    int     created_at;
} __attribute__((aligned(8))) Card;

#define PERM_DATA_SIZE 256
typedef struct Perm {
    Key         giver;
    Key         recipient;
    Nonce       nonce;
    uint8_t     data[PERM_DATA_SIZE];
    Signature   signature;
} __attribute__((aligned(8))) Perm;
HashT hash_perm(Perm* p);
size_t marshal_perm(uint8_t* pb, Perm* p);
size_t unmarshal_perm(uint8_t* pb, Perm* p);


#define VOUCH_DATA_SIZE 256
typedef struct Voucher {
    Key         to;
    Key         from;

    HashT       file_hash;
    size_t      file_size;
    time_t      expiration;

    unsigned char   data[VOUCH_DATA_SIZE];
    Signature       signature;

} __attribute__((aligned(8))) Voucher;
HashT hash_voucher(Voucher* v);
size_t marshal_voucher(uint8_t* pb, Voucher* v);
size_t unmarshal_voucher(uint8_t* pb, Voucher* v);

#ifdef __cplusplus
}
#endif

