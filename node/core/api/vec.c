/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz/api/vec.h"
#include <stdlib.h>

Vec* new_vec(size_t cap) {
    uint8_t* bytes = (uint8_t*)malloc(cap);
    Vec* v = malloc(sizeof(Vec));
    v->b = bytes;
    v->c = bytes;
    v->len = 0;
    v->cap = cap;
    return v;
}

void free_vec(Vec* v) {
    if (!v) return;
    if (v->b != NULL) free(v->b);
    free(v);
}
