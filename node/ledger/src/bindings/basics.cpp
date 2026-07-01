/*
 * Bullet Ledger
 * Copyright (C) 2025 Joshua Olson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ledger/ledger.h"
#include "sz/codec.h"
#include <cstring>

int ledger_create_account(
    void* ledger,
    const unsigned char* key, size_t key_size,
    const HashT* block_hash,
    const HashT* prev_block_hash
) {
    if (!ledger || !key || !block_hash) return NULL_PARAMETER;

    auto l = reinterpret_cast<Ledger*>(ledger);
    const ByteSlice key_slice((byte*)key, key_size);

    Hash bh;
    memcpy(bh.h, block_hash->b, HASH_SIZE);

    Hash pbh;
    memcpy(pbh.h, prev_block_hash->b, HASH_SIZE);

    return l->create_account(key_slice, &bh, &pbh);
}

int ledger_delete_account(
    void* ledger,
    const unsigned char* key, size_t key_size,
    const HashT* block_hash,
    const HashT* prev_block_hash
) {
    if (!ledger || !key || !block_hash) return NULL_PARAMETER;

    auto l = reinterpret_cast<Ledger*>(ledger);
    const ByteSlice key_slice((byte*)key, key_size);

    Hash bh;
    memcpy(bh.h, block_hash->b, HASH_SIZE);

    Hash pbh;
    memcpy(pbh.h, prev_block_hash->b, HASH_SIZE);

    return l->delete_account(key_slice, &bh, &pbh);
}


int ledger_put(
    void* ledger, 
    const unsigned char* key, size_t key_size,
    const HashT* value_hash, uint8_t val_idx,
    const HashT* block_hash,
    const HashT* prev_block_hash
) {
    if (!ledger || !key || !block_hash || !value_hash) return NULL_PARAMETER;
    if (val_idx > LEAF_ORDER) return VAL_IDX_RANGE;

    auto l = reinterpret_cast<Ledger*>(ledger);

    const ByteSlice key_slice((byte*)key, key_size);

    Hash bh;
    memcpy(bh.h, block_hash->b, HASH_SIZE);

    Hash pbh;
    memcpy(pbh.h, prev_block_hash->b, HASH_SIZE);

    Hash vh;
    memcpy(vh.h, value_hash->b, HASH_SIZE);

    return l->put(key_slice, &vh, val_idx, &bh, &pbh);
}

int ledger_replace(
    void* ledger, 
    const unsigned char* key, size_t key_size,
    const HashT* value_hash, uint8_t val_idx,
    const HashT* prev_value_hash,
    const HashT* block_hash,
    const HashT* prev_block_hash
) {
    if (!ledger || 
        !value_hash || 
        !key || 
        !prev_value_hash || 
        !block_hash
    ) return NULL_PARAMETER;
    if (val_idx > LEAF_ORDER) return VAL_IDX_RANGE;

    auto l = reinterpret_cast<Ledger*>(ledger);

    const ByteSlice key_slice((byte*)key, key_size);

    Hash bh;
    memcpy(bh.h, block_hash->b, HASH_SIZE);

    Hash pbh;
    memcpy(pbh.h, prev_block_hash->b, HASH_SIZE);

    Hash vh;
    memcpy(vh.h, value_hash->b, HASH_SIZE);

    Hash pvh;
    memcpy(pvh.h, prev_value_hash->b, HASH_SIZE);

    return l->replace(key_slice, &vh, &pvh, val_idx, &bh, &pbh);
}

int ledger_remove(
    void* ledger, 
    const unsigned char* key, size_t key_size,
    uint8_t val_idx,
    const HashT* block_hash,
    const HashT* prev_block_hash 
) {
    if (!ledger || !key || !block_hash) return NULL_PARAMETER;
    if (val_idx < LEAF_ORDER) return VAL_IDX_RANGE;

    auto l = reinterpret_cast<Ledger*>(ledger);

    const ByteSlice key_slice((byte*)key, key_size);

    Hash zero_h;
    std::memset(zero_h.h, 0, 32);

    Hash bh;
    memcpy(bh.h, block_hash->b, HASH_SIZE);

    Hash pbh;
    memcpy(pbh.h, prev_block_hash->b, HASH_SIZE);
    return l->put(key_slice, &zero_h, val_idx, &bh, &pbh);
}
