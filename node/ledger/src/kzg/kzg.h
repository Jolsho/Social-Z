/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#pragma once
#include "utils/hashing.h"
#include "kzg/settings.h"
#include <optional>

using Scalar_vec = std::vector<blst_scalar>;

std::optional<blst_p1> prove_kzg(
    const Scalar_vec &evals,
    const size_t eval_idx,
    const KZGSettings &s
);


bool verify_kzg(
    const blst_p1 C, 
    const blst_scalar z, 
    const blst_scalar y, 
    const blst_p1 Pi, 
    const SRS &S
);

bool batch_verify(
    std::vector<blst_p1> &Pis,
    std::vector<blst_p1> &Cs,
    std::vector<size_t> &Z_idxs,
    Scalar_vec &Ys,
    Hash base_r,
    const KZGSettings &kzg
);
