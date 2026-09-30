/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_common/codec.h"

typedef struct {
    uint64_t priority;
    HashT h;
} PQNode;

int compare_pqnode(void* n1, void* n2);
