/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#pragma once
#include "lmdb.h"
#include <shared_mutex>
#include <vector>

class BulletDB {
public:
    MDB_env* env_;
    MDB_dbi dbi_;
    int count_;
    std::shared_mutex mux_;

    BulletDB(const char* path, size_t map_size);
    ~BulletDB();
    void* start_txn();
    void* start_rd_txn();

    void end_txn(void* trx, int rc = 0);

    int put(const void* key_data, size_t key_size, 
            const void* value_data, size_t value_size,
            void* trx
        );
    int get(
        const void* key_data, size_t key_size, 
        std::vector<std::byte> &out, 
        void* trx
    );
    int get_raw(const void* key_data, size_t key_size,
            void** value_data, size_t* value_size,
             void* trx
    );

    int del(const void* key_data, size_t key_size,  void* trx);
    int exists(const void* key_data, size_t key_size,  void* trx);
    std::vector<uint64_t> flatten_sort_l2();
};
