#include "utils/db.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <lmdb.h>

LMDB::LMDB(const char* path, size_t map_size) {
    assert(mdb_env_create(&env_) == 0);
    assert(mdb_env_set_mapsize(env_, map_size) == 0);
    assert(mdb_env_open(env_, path, 0, 0600) == 0);

    MDB_txn* trx = start_txn();
    assert(mdb_dbi_open(trx, nullptr, 0, &dbi_) == 0);
    end_txn(trx);
}

LMDB::~LMDB() {
    mdb_dbi_close(env_, dbi_);
    mdb_env_close(env_);
}

MDB_txn* LMDB::start_txn() { 
    MDB_txn* tx;
    assert(mdb_txn_begin(env_, nullptr, 0, &tx) == 0); 
    return tx;
}

MDB_txn* LMDB::start_rd_txn() { 
    MDB_txn* tx;
    assert(mdb_txn_begin(env_, nullptr, MDB_RDONLY, &tx) == 0); 
    return tx;
}

void LMDB::end_txn(MDB_txn* trx, int rc) {
    if (rc == 0) 
        assert(mdb_txn_commit(trx) == 0);
    else 
        mdb_txn_abort(trx);
}

int LMDB::put(
    const void* key_data, size_t key_size, 
    const void* value_data, size_t value_size, 
    MDB_txn* trx
) {
    MDB_val key{ key_size, (void*)(key_data) };
    MDB_val value{ value_size, (void*)(value_data) };

    return mdb_put(trx, dbi_, &key, &value, 0);
}

int LMDB::get_raw(
    const void* key_data, size_t key_size, 
    void** value_data, size_t* value_size,
    MDB_txn* trx
) {
    MDB_val key{ key_size, (void*)(key_data) };
    MDB_val value;

    int rc = mdb_get(trx, dbi_, &key, &value);
    if (rc == 0) {
        *value_size = value.mv_size;
        *value_data = malloc(value.mv_size);
        memcpy(*value_data, value.mv_data, value.mv_size);
    }
    return rc;
}

int LMDB::get(
    const void* key_data, size_t key_size, 
    std::vector<std::byte> &out,
    MDB_txn* trx
) {
    MDB_val key{ key_size, (void*)(key_data) };
    MDB_val value;

    int rc = mdb_get(trx, dbi_, &key, &value);
    if (rc == 0) {
        out.resize(value.mv_size);
        std::memcpy(out.data(), value.mv_data, value.mv_size);
    }
    return rc;
}

int LMDB::del(const void* key_data, size_t key_size, MDB_txn* trx) {
    MDB_val key{ key_size, (void*)(key_data) };
    return mdb_del(trx, dbi_, &key, nullptr);
}

int LMDB::exists(const void* key_data, size_t key_size, MDB_txn* trx) {
    MDB_val key{ key_size, (void*)(key_data) };
    MDB_val value;
    return mdb_get(trx, dbi_, &key, &value);
}
