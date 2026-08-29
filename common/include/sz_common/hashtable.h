/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Portions of this file are derived from:
 * goldsborough/hashtable
 * https://github.com/goldsborough/hashtable
 *
 * Copyright (c) 2016 Peter Goldsborough
 *
 * The portions derived from goldsborough/hashtable are licensed under
 * the MIT License. The MIT license notice is retained below:
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of
 *  this software and associated documentation files (the "Software"), to deal in
 *  the Software without restriction, including without limitation the rights to
 *  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 *  the Software, and to permit persons to whom the Software is furnished to do so,
 *  subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 *  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 *  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 *  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

#pragma once

#include "sz_common/codec.h"
#include <stdbool.h>
#include <stddef.h>

/****************** DEFINTIIONS ******************/

#define HT_ERROR -1
#define HT_SUCCESS 0

#define HT_UPDATED 1
#define HT_INSERTED 0

#define HT_NOT_FOUND 0


/****************** STRUCTURES ******************/

typedef struct HTNode {
	HashT   key;
	void*   value;

	struct HTNode* next;
} HTNode;

typedef void (*DestroyValue)(void* ctx, void* value);
typedef struct FreeValueCallback {
    DestroyValue destroy;
    void*       ctx;
} FreeValueCallback;

typedef struct HashTable {
    uint8_t*    _b;

	size_t value_size;

	HTNode**    nodes;
	size_t      size;
    size_t      cap;
    size_t      base_cap;


    FreeValueCallback*  val_c;
	HTNode*      free_list;

    HTNode*     pool;
    uint8_t*    value_pool;

} HashTable;


/****************** INTERFACE ******************/


/* Setup */
int ht_setup(HashTable* table, FreeValueCallback* vc, size_t value_size, size_t capacity);


static inline size_t ht_memory_overhead(size_t value_size, size_t capacity) { 
    return (sizeof(HTNode) + 8 + value_size) *  capacity;                
}


/* Destructor */
int ht_destroy(HashTable* table);

int ht_insert(HashTable* table, HashT* key, void* value);
void* ht_lookup(HashTable* table, HashT* key);
const void* ht_const_lookup(const HashTable* table, HashT* key);
void* ht_reserve(HashTable* table, HashT* key);

#define HT_LOOKUP_AS(type, table_pointer, key_pointer) \
	((type*)ht_lookup((table_pointer), (key_pointer)))

int ht_erase(HashTable* table, HashT* key);
int ht_clear(HashTable* table);

int ht_is_empty(HashTable* table);
bool ht_is_initialized(HashTable* table);


/****************** PRIVATE ******************/

static inline void _ht_hash(const HashTable* table, HashT* key, size_t* h) {
    memcpy(h, key->b, sizeof(size_t));
	*h = *h % table->base_cap;
}
static inline bool _ht_equal(const HashTable* table, const HashT* first_key, const HashT* second_key) {
	return memcmp(first_key->b, second_key->b, HASH_SIZE) == 0;
}

static inline bool _ht_is_full(HashTable* table) {
	assert(table->size <= table->cap);
	return table->size == table->cap;
}

HTNode* _ht_create_node(HashTable* table, HashT* key, void* value, HTNode* next);
void _ht_free_node(HashTable* table, HTNode* node);

int _ht_allocate(HashTable* table);
static void _ht_lru_move_front(HashTable *table, HTNode *node);
static void _ht_lru_push_front(HashTable *table, HTNode *node);

#endif /* HASHTABLE_H */
