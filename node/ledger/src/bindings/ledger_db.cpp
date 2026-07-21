/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#include "ledger/ledger.h"

extern "C" {
int ledger_db_store_value(
    void* ledger, 
    const unsigned char* key, size_t key_size,
    const unsigned char* value, size_t value_size
) {
    if (!ledger || !value || !key) return NULL_PARAMETER;
    auto l = reinterpret_cast<Ledger*>(ledger);

    const ByteSlice key_slice((byte*)key, key_size);
    Hash key_hash;
    derive_hash(key_hash.h, key_slice);

    const ByteSlice value_slice((byte*)value, value_size);

    return l->store_value(&key_hash, value_slice);
}

int ledger_db_delete_value(
    void* ledger, 
    const unsigned char* key, size_t key_size
) {
    if (!ledger || !key) return NULL_PARAMETER;
    auto l = reinterpret_cast<Ledger*>(ledger);

    const ByteSlice key_slice((byte*)key, key_size);
    Hash key_hash;
    derive_hash(key_hash.h, key_slice);

    return l->delete_value(&key_hash);
}

int ledger_db_get_value(
    void* ledger, 
    const unsigned char* key, size_t key_size,
    void** out, size_t* out_size
) {
    if (!ledger || !key) return NULL_PARAMETER;
    auto l = reinterpret_cast<Ledger*>(ledger);

    const ByteSlice key_slice((byte*)key, key_size);
    Hash key_hash;
    derive_hash(key_hash.h, key_slice);

    return l->get_value(&key_hash, out, out_size);
}

int ledger_db_value_exists(
    void* ledger, 
    const unsigned char* key, size_t key_size
) {
    if (!ledger || !key) return NULL_PARAMETER;
    auto l = reinterpret_cast<Ledger*>(ledger);

    const ByteSlice key_slice((byte*)key, key_size);
    Hash key_hash;
    derive_hash(key_hash.h, key_slice);

    return l->value_exists(&key_hash);
}
}
