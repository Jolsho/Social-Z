/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz/client/posts.h"
#include <string.h>
#include <stdio.h>

bool marshal_request(UserCtx* ctx, void* r, size_t r_size, uint8_t* buff, size_t len) {
    if (len < r_size) return false;
    memcpy(buff, (uint8_t*)r, r_size);
    free(r);
    return true;
}

char* post_set_created_at(Post* c) { 
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    snprintf(
        c->created_at, 9,
        "%02d/%02d/%02d",
        tm.tm_mday,
        tm.tm_mon + 1,
        tm.tm_year % 100
    );
    return c->created_at;
}
