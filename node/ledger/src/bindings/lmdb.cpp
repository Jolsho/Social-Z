
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

#include "db.h"
#include "state_types.h"

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
