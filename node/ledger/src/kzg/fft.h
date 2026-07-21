/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include <vector>
#include "blst.h"

void fft_in_place( 
    std::vector<blst_scalar> &a, 
    const std::vector<blst_scalar> &roots 
);
void inverse_fft_in_place(
    std::vector<blst_scalar> &a, 
    const std::vector<blst_scalar> &inv_roots
);
