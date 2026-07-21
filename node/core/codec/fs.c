/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz/codec.h"
#include "sz/hash.h"
#include <string.h>

size_t marshal_perm(uint8_t* pb, Perm* p) {
    memcpy(pb, p, sizeof(Perm));
    return sizeof(Perm);
}
size_t unmarshal_perm(uint8_t* pb, Perm* p) {
    memcpy(p, pb, sizeof(Perm));
    return sizeof(Perm);
}
HashT hash_perm(Perm* p) {
    Hasher h = new_hasher();
    hash_update(&h, p->giver.b, KEY_SIZE);
    hash_update(&h, p->recipient.b, KEY_SIZE);
    hash_update(&h, p->nonce.b, NONCE_SIZE);
    hash_update(&h, p->data, PERM_DATA_SIZE);
    return hash_finalize(&h);
}

size_t marshal_voucher(uint8_t* pb, Voucher* v) {
    memcpy(pb, v, sizeof(Voucher));
    return sizeof(Voucher);
}
size_t unmarshal_voucher(uint8_t* pb, Voucher* v) {
    memcpy(v, pb, sizeof(Voucher));
    return sizeof(Voucher);
}
HashT hash_voucher(Voucher* v) {
    Hasher h = new_hasher();
    hash_update(&h, v->to.b, KEY_SIZE);
    hash_update(&h, v->from.b, KEY_SIZE);
    hash_update(&h, v->file_hash.b, HASH_SIZE);

    hash_update(&h, (uint8_t*)(&v->expiration), sizeof(time_t));
    hash_update(&h, v->data, VOUCH_DATA_SIZE);
    return hash_finalize(&h);
}
