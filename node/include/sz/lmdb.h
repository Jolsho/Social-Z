#pragma once
#include <lmdb.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LMDB{
    MDB_env* env_;
    MDB_dbi dbi_;
    int count_;
} LMDB;

LMDB* new_lmdb(const char* path, size_t map_size);
void close_lmdb(LMDB* db);

MDB_txn* start_txn(LMDB* db);
MDB_txn* start_rd_txn(LMDB* db);
void end_txn(LMDB* db, MDB_txn* trx, int rc);

int put(LMDB* db, const uint8_t* key_data, size_t key_size, 
    const uint8_t* value_data, size_t value_size,
    MDB_txn* trx
);

int get(LMDB* db,
    const uint8_t* key_data, size_t key_size, 
    uint8_t** out, size_t* out_size,
    MDB_txn* trx
);

int get_raw(LMDB* db, const uint8_t* key_data, size_t key_size,
        uint8_t** value_data, size_t* value_size,
         MDB_txn* trx
);

int del(LMDB* db, const uint8_t* key_data, size_t key_size,  MDB_txn* trx);
int exists(LMDB* db, const uint8_t* key_data, size_t key_size,  MDB_txn* trx);

#ifdef __cplusplus
}
#endif
