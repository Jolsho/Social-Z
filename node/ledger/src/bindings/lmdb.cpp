/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#include "ledger/db.h"
#include "trie/state_types.h"

extern "C" {

////////////////////////////
///////// LMDB ////////////
//////////////////////////
int lmdb_open(
    void** out,
    const char* path, 
    size_t map_size
) {
    *out = new BulletDB(path, map_size);
    return OK;
}

int lmdb_put(
    void* db, 
    const unsigned char* key, size_t key_size,
    const unsigned char* value, size_t value_size
) {
    auto db_ = reinterpret_cast<BulletDB*>(db);

    void* trx = db_->start_txn();
    int rc = db_->put(
        key, key_size, 
        value, value_size, 
        trx
    );
    db_->end_txn(trx, rc);

    return rc;
}

int lmdb_delete(
    void* db, 
    const unsigned char* key, size_t key_size
) {
    auto db_ = reinterpret_cast<BulletDB*>(db);

    void* trx = db_->start_txn();
    int rc = db_->del(key, key_size, trx);
    db_->end_txn(trx, rc);

    return rc;
}

void lmdb_free(
    const unsigned char* data, 
    size_t data_size
) { 
    free((void*)data); 
}

int lmdb_get(
    void* db, 
    const unsigned char* key, size_t key_size,
    void** out, size_t* out_size
) {
    auto db_ = reinterpret_cast<BulletDB*>(db);

    void* trx = db_->start_txn();
    int rc = db_->get_raw(key, key_size, out, out_size, trx);
    db_->end_txn(trx, rc);

    return rc;
}

int lmdb_exists(
    void* db, 
    const unsigned char* key, size_t key_size
) {
    auto db_ = reinterpret_cast<BulletDB*>(db);

    void* trx = db_->start_txn();
    int rc = db_->exists(key, key_size, trx);
    db_->end_txn(trx, rc);

    return rc;
}
}
