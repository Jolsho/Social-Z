#pragma once
#include "lmdb.h"
#include <vector>

class DB {
public:
    MDB_env* env_;
    MDB_dbi dbi_;
    int count_;

    DB(const char* path, size_t map_size);
    ~DB();
    MDB_txn* start_txn();
    MDB_txn* start_rd_txn();

    void end_txn(MDB_txn* trx, int rc = 0);

    int put(const void* key_data, size_t key_size, 
        const void* value_data, size_t value_size,
        MDB_txn* trx
    );

    int get(
        const void* key_data, size_t key_size, 
        std::vector<std::byte> &out, 
        MDB_txn* trx
    );

    int get_raw(const void* key_data, size_t key_size,
            void** value_data, size_t* value_size,
             MDB_txn* trx
    );

    int del(const void* key_data, size_t key_size,  MDB_txn* trx);
    int exists(const void* key_data, size_t key_size,  MDB_txn* trx);
};
