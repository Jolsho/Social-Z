#ifndef RAY_H
#define RAY_H

#include "vec3.h"

typedef struct {
    Vec3 origin;
    Vec3 direction;
} Ray;

static inline Vec3 ray_at(Ray ray, float t)
{
    return vec3_add(
        ray.origin,
        vec3_mul(ray.direction, t)
    );
}

#endif
