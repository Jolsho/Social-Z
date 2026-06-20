
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

// db.h
#include <cstddef>

extern "C" {
    int lmdb_open(
        void** out,
        const char* path, 
        size_t cache_size,
        size_t map_size,
        const char* tag,
        unsigned char* secret,
        size_t secret_size
    );

    int lmdb_put(
        void* db, 
        const unsigned char* key_hash, size_t key_hash_size,
        const unsigned char* value, size_t value_size
    );

    int lmdb_delete(
        void* db, 
        const unsigned char* key, size_t key_size
    );

    void lmdb_free(const unsigned char* data);

    int lmdb_get(
        void* db, 
        const unsigned char* key, size_t key_size,
        void** out, size_t* out_size
    );

    int lmdb_exists(
        void* db, 
        const unsigned char* key, size_t key_size
    );
}
