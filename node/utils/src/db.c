/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_node/utils/db.h"
#include <assert.h>
#include <stdlib.h>
#include <memory.h>

LMDB* new_lmdb(const char* path, size_t map_size) {
    LMDB* db = (LMDB*)malloc(sizeof(LMDB));

    assert(mdb_env_create(&db->env_) == 0);
    assert(mdb_env_set_mapsize(db->env_, map_size) == 0);
    assert(mdb_env_open(db->env_, path, 0, 0600) == 0);

    MDB_txn* trx = start_txn(db);
    assert(mdb_dbi_open(trx, NULL, 0, &db->dbi_) == 0);
    end_txn(db, trx, 0);

    return db;
}

void close_lmdb(LMDB* db) {
    mdb_dbi_close(db->env_, db->dbi_);
    mdb_env_close(db->env_);
}

MDB_txn* start_txn(LMDB* db) { 
    MDB_txn* tx;
    assert(mdb_txn_begin(db->env_, NULL, 0, &tx) == 0); 
    return tx;
}

MDB_txn* start_rd_txn(LMDB* db) { 
    MDB_txn* tx;
    assert(mdb_txn_begin(db->env_, NULL, MDB_RDONLY, &tx) == 0); 
    return tx;
}

void end_txn(LMDB* db, MDB_txn* trx, int rc) {
    if (rc == 0) 
        assert(mdb_txn_commit(trx) == 0);
    else 
        mdb_txn_abort(trx);
}

int put(LMDB* db,
    const uint8_t* key_data, size_t key_size, 
    const uint8_t* value_data, size_t value_size, 
    MDB_txn* trx
) {
    MDB_val key;
    key.mv_size = key_size;
    key.mv_data = (void*)key_data;

    MDB_val value;
    value.mv_size = value_size;
    value.mv_data = (void*)value_data;

    return mdb_put(trx, db->dbi_, &key, &value, 0);
}


int get(LMDB* db,
    const uint8_t* key_data, size_t key_size, 
    uint8_t** out, size_t* out_size,
    MDB_txn* trx
) {
    MDB_val key;
    key.mv_size = key_size;
    key.mv_data = (void*)key_data;

    MDB_val value;

    int rc = mdb_get(trx, db->dbi_, &key, &value);
    if (rc == 0) {
        *out = malloc(value.mv_size);
        *out_size = value.mv_size;
        memcpy(*out, value.mv_data, value.mv_size);
    }
    return rc;
}

int del(LMDB* db, const uint8_t* key_data, size_t key_size, MDB_txn* trx) {
    MDB_val key;
    key.mv_size = key_size;
    key.mv_data = (void*)key_data;
    return mdb_del(trx, db->dbi_, &key, NULL);
}

int exists(LMDB* db, const uint8_t* key_data, size_t key_size, MDB_txn* trx) {
    MDB_val key;
    key.mv_size = key_size;
    key.mv_data = (void*)key_data;
    MDB_val value;
    return mdb_get(trx, db->dbi_, &key, &value);
}
