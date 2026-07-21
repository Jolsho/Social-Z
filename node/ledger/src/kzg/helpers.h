/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "utils/bigint.h"
#include "blst.h"

blst_scalar num_scalar(const uint64_t v);

const blst_scalar ZERO_SK = num_scalar(0);
const blst_scalar ONE_SK = num_scalar(1);

bool scalar_is_zero(const blst_scalar &s);
bool equal_scalars(const blst_scalar &a, const blst_scalar &b);
void print_scalar(blst_scalar* s);
void print_p1(blst_p1* p);
blst_p1 p1_from_bytes(const byte* buff);
blst_p2 p2_from_bytes(const byte* buff);
blst_scalar modular_pow(const blst_scalar &base, const BigInt &exp);
void hash_to_sk(blst_scalar* dst, const byte* hash);
blst_p1 new_p1();
blst_p1 new_inf_p1();
blst_p2 new_p2();
blst_p2 new_inf_p2();
