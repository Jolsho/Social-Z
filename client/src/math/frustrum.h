/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#pragma once
#include "math/aabb.h"
#include "math/vec3.h"
#include <stdbool.h>

typedef struct {
    Vec3 normal;
    float distance;
} Plane;

typedef struct {
    Plane planes[6];
} Frustum;

static bool aabb_inside_frustum(
    AABB box,
    Frustum frustum
)
{
    for (int i = 0; i < 6; i++) {
        Plane p = frustum.planes[i];

        Vec3 positive;

        for (int i = 0; i < 3; i++) {
            positive.v[i] =
                p.normal.v[i] >= 0.0f
                ? box.max.v[i]
                : box.min.v[i];
        }

        float distance =
            p.normal.v[0] * positive.v[0] +
            p.normal.v[1] * positive.v[1] +
            p.normal.v[2] * positive.v[2] +
            p.distance;

        if (distance < 0.0f)
            return false;
    }

    return true;
}

