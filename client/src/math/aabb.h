#ifndef AABB_H
#define AABB_H

#include "vec3.h"
#include "ray.h"
#include <stdbool.h>

typedef struct {
    Vec3 min;
    Vec3 max;
} AABB;

static inline AABB aabb(Vec3 min, Vec3 max)
{
    return (AABB){
        .min = min,
        .max = max
    };
}

int aabb_ray_intersect(
    AABB box,
    Ray ray,
    float *t_near,
    float *t_far
);

static inline float aabb_surface_area(AABB a)
{
    float x = a.max.v[0] - a.min.v[0];
    float y = a.max.v[1] - a.min.v[1];
    float z = a.max.v[2]- a.min.v[2];
    return 2.0f * (x*y + y*z + z*x);
}

static inline AABB aabb_union(AABB a, AABB b)
{
    AABB r;

    r.min.v[0] = a.min.v[0] < b.min.v[0] ? a.min.v[0] : b.min.v[0];
    r.max.v[0] = a.max.v[0] > b.max.v[0] ? a.max.v[0] : b.max.v[0];

    r.min.v[1] = a.min.v[1] < b.min.v[1] ? a.min.v[1] : b.min.v[1];
    r.max.v[1] = a.max.v[1] > b.max.v[1] ? a.max.v[1] : b.max.v[1];

    r.min.v[2] = a.min.v[2] < b.min.v[2] ? a.min.v[2] : b.min.v[2];
    r.max.v[2] = a.max.v[2] > b.max.v[2] ? a.max.v[2] : b.max.v[2];

    return r;
}

static inline bool aabb_contains(AABB outer, AABB inner)
{
    return
        inner.min.v[0] >= outer.min.v[0] &&
        inner.min.v[1] >= outer.min.v[1] &&
        inner.min.v[2] >= outer.min.v[2] &&
        inner.max.v[0] <= outer.max.v[0] &&
        inner.max.v[1] <= outer.max.v[1] &&
        inner.max.v[2] <= outer.max.v[2];
}

static inline AABB aabb_fatten(AABB a, float margin)
{
    a.min.v[0] -= margin;
    a.max.v[0] += margin;

    a.min.v[1] -= margin;
    a.max.v[1] += margin;

    a.min.v[2] -= margin;
    a.max.v[2] += margin;

    return a;
}

static inline bool aabb_overlap(AABB a, AABB b)
{
    return
        a.min.v[0] <= b.max.v[0] &&
        a.max.v[0] >= b.min.v[0] &&
        a.min.v[1] <= b.max.v[1] &&
        a.max.v[1] >= b.min.v[1] &&
        a.min.v[2] <= b.max.v[2] &&
        a.max.v[2] >= b.min.v[2];
}

static inline bool sphere_aabb_overlap(Vec3 center, float radius, AABB box)
{
    float distance_sq = 0.0f;
    for (int i = 0; i < 3; i++) {
        float v = center.v[i];
        if (v < box.min.v[i]) {
            float d = box.min.v[i] - v;
            distance_sq += d * d;
        }
        else if (v > box.max.v[i]) {
            float d = v - box.max.v[0];
            distance_sq += d * d;
        }
    }
    return distance_sq <= radius * radius;
}
#endif

