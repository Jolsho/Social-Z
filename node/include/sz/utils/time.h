/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include <ctime>

inline time_t next_midnight() {
    time_t now = time(nullptr);

    tm t = *localtime(&now);

    // Move to tomorrow.
    t.tm_mday += 1;

    // Set to midnight.
    t.tm_hour = 0;
    t.tm_min  = 0;
    t.tm_sec  = 0;

    time_t next_midnight = mktime(&t);

    // Ensure it's more than 24 hours ahead.
    if (next_midnight <= now + 24 * 60 * 60) {
        t.tm_mday += 1;
        next_midnight = mktime(&t);
    }

    return next_midnight;
}
