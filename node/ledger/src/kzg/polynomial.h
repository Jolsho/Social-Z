/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#pragma once
#include "kzg/settings.h"
#include <optional>
 
using Polynomial = std::vector<blst_scalar>;

void commit_g1(blst_p1* C, const Polynomial& coeffs, const SRS& srs);

Polynomial multiply_binomial(
    const Polynomial &P,
    const blst_scalar &w
);

Polynomial differentiate_polynomial(const Polynomial &f);

std::optional<Polynomial> derive_quotient(
    const std::vector<blst_scalar> &poly_eval,
    const blst_scalar &z,
    const blst_scalar &y,
    const NTTRoots &roots
);
