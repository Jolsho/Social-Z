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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sz/utils/hashtable.h"

int ht_setup(HashTable* table, FreeValueCallback* vc, size_t value_size, size_t capacity) {
	assert(table != NULL);

    if (capacity == 0 || table == NULL) return HT_ERROR;

    table->base_cap = capacity < 512 ? capacity : 512;
    table->cap = capacity;
	table->value_size = value_size;
    table->val_c = vc;
	table->size = 0;

	if (_ht_allocate(table) == HT_ERROR) {
		return HT_ERROR;
	}

	return HT_SUCCESS;
}

int ht_destroy(HashTable* table) {
	size_t chain;
    HTNode* next;
    HTNode* curr;

	assert(ht_is_initialized(table));
	if (!ht_is_initialized(table)) return HT_ERROR;

    for (size_t i = 0; i < table->cap; ++i) {
        if (table->val_c)
            table->val_c->destroy(table->val_c->ctx, table->pool[i].value);
    }

	free(table->_b);

    table->nodes = NULL;
    table->pool = NULL;
    table->free_list = NULL;
    table->size = 0;

	return HT_SUCCESS;
}

int ht_insert(HashTable* table, HashT* key, void* value) {
	size_t index;
	HTNode* node;
	HTNode* prev = NULL;

	if (!ht_is_initialized(table)) return HT_ERROR;
	if (key == NULL) return HT_ERROR;

    _ht_hash(table, key, &index);

    /* Update existing entry */
    for (node = table->nodes[index]; node; node = node->next) {
        if (_ht_equal(table, key, &node->key)) {
            memcpy(node->value, value, table->value_size);
            return HT_UPDATED;
        }
    }

    /* Insert at head of hash chain */
    node = _ht_create_node(table, key, value, table->nodes[index]);
    if (!node) return HT_ERROR;

    table->nodes[index] = node;
    ++table->size;

	return HT_INSERTED;
}

void* ht_lookup(HashTable* table, HashT* key) {
	HTNode* node;
	size_t index;

	assert(table != NULL);
	assert(key != NULL);

	if (table == NULL) return NULL;
	if (key == NULL) return NULL;

    _ht_hash(table, key, &index);
	for (node = table->nodes[index]; node; node = node->next) {
		if (_ht_equal(table, key, &node->key)) {
			return node->value;
		}
	}

	return NULL;
}

const void* ht_const_lookup(const HashTable* table, HashT* key) {
	const HTNode* node;
	size_t index;

	assert(table != NULL);
	assert(key != NULL);

	if (table == NULL) return NULL;
	if (key == NULL) return NULL;

    _ht_hash(table, key, &index);

	for (node = table->nodes[index]; node; node = node->next) {
		if (_ht_equal(table, key, &node->key)) {
			return node->value;
		}
	}

	return NULL;
}

void* ht_reserve(HashTable* table, HashT* key) {
	size_t index;
	HTNode* node;
	HTNode* prev = NULL;

	if (!ht_is_initialized(table)) return NULL;
	if (key == NULL) return NULL;

    _ht_hash(table, key, &index);

        /* Update existing entry */
    for (node = table->nodes[index]; node; node = node->next) {
        if (_ht_equal(table, key, &node->key)) {
            return node->value;
        }
    }

    /* Insert at head of hash chain */
    node = _ht_create_node(table, key, NULL, table->nodes[index]);
    if (!node) return NULL;

    table->nodes[index] = node;
    ++table->size;

    return node;
}

int ht_erase(HashTable* table, HashT* key) {
	HTNode* node;
	HTNode* previous;
	size_t index;

	assert(table != NULL);
	assert(key != NULL);

	if (table == NULL) return HT_ERROR;
	if (key == NULL) return HT_ERROR;

    _ht_hash(table, key, &index);

	node = table->nodes[index];

	for (previous = NULL; node; previous = node, node = node->next) {
		if (_ht_equal(table, key, &node->key)) {
			if (previous) {
				previous->next = node->next;

			} else if (node->next) {
                HTNode* next = node->next;
				table->nodes[index] = next;

            } else {
                table->nodes[index] = NULL;
			}
			_ht_free_node(table, node);

			--table->size;

			return HT_SUCCESS;
		}
	}

	return HT_NOT_FOUND;
}

int ht_clear(HashTable* table) {
	assert(table != NULL);
	assert(table->nodes != NULL);

	if (table == NULL) return HT_ERROR;
	if (table->nodes == NULL) return HT_ERROR;


	ht_destroy(table);
	if (_ht_allocate(table) == HT_ERROR) return HT_ERROR;
	table->size = 0;

	return HT_SUCCESS;
}

int ht_is_empty(HashTable* table) {
	assert(table != NULL);
	if (table == NULL) return HT_ERROR;
	return table->size == 0;
}

bool ht_is_initialized(HashTable* table) {
	return table != NULL && table->nodes != NULL;
}

/****************** PRIVATE ******************/



HTNode* _ht_create_node(HashTable* table, HashT* key, void* value, HTNode* next) {
	HTNode* node;

	assert(table != NULL);
	assert(key != NULL);

    node = table->free_list;
	if (!node) { return NULL; }
    table->free_list = node->next;
    node->next = NULL;

	memcpy(&node->key, key, HASH_SIZE);
    if (value) memcpy(node->value, value, table->value_size);
	node->next = next;

	return node;
}

void _ht_free_node(HashTable* table, HTNode* node) {
	assert(node != NULL);
    if (table->val_c) 
        table->val_c->destroy(table->val_c->ctx, node->value);

    void* value = node->value;
	memset(value, 0, table->value_size);
	memset(node, 0, sizeof(HTNode));

    node->value = value;
    node->next = table->free_list;
    table->free_list = node;
}

int _ht_allocate(HashTable* table) {
    size_t total;
    total += table->base_cap * sizeof(HTNode*);
    total += table->cap * sizeof(HTNode);
    total += table->cap * table->value_size;

    uint8_t* b = malloc(total);
    if (!b) return HT_ERROR;
	memset(b, 0, total);

    table->nodes = (HTNode**)b;
    b += table->base_cap * sizeof(HTNode*);

    table->pool = (HTNode*)b;
    b += table->cap * sizeof(HTNode);

    table->_b = b;
    b += table->cap * table->value_size;

    table->free_list = NULL;

    uint8_t *value = b;
    HTNode* pool = table->pool;
    for (size_t i = 0; i < table->cap; ++i) {
        pool[i].value = value;
        pool[i].next = table->free_list;
        table->free_list = &pool[i];

        value += table->value_size;
    }

	return HT_SUCCESS;
}
