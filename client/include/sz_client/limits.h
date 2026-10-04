/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

// Complete encrypted package, including envelope and authenticated chunk framing.
#define SZ_PACKAGE_CIPHERTEXT_MAX (64ULL * 1024 * 1024)

// Ciphertext cache budget, including its initial index overhead; excludes plaintext packages.
#define SZ_CIPHERTEXT_CACHE_BUDGET (128ULL * 1024 * 1024)
