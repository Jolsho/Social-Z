#include "aabb.h"
#include <math.h>

int aabb_ray_intersect(
    AABB box,
    Ray ray,
    float *t_near,
    float *t_far)
{
    float tmin = -INFINITY;
    float tmax =  INFINITY;

    // First: eliminate impossible parallel axes.
    for (int i = 0; i < 3; i++) {
        if (ray.direction.v[i] == 0.0f &&
            (ray.origin.v[i] < box.min.v[i] || ray.origin.v[i] > box.max.v[i])
        ) return 0;
    }

    float inv, t1, t2, tmp;

    for (int i = 0; i < 3; i++) {
        /* X, Y, Z */
        inv = 1.0f / ray.direction.v[i];
        t1 = (box.min.v[i] - ray.origin.v[i]) * inv;
        t2 = (box.max.v[i] - ray.origin.v[i]) * inv;
        if (t1 > t2) {
            tmp = t1;
            t1 = t2;
            t2 = tmp;
        }
        if (t1 > tmin) tmin = t1;
        if (t2 < tmax) tmax = t2;
        if (tmin > tmax) return 0;
    }

    if (t_near) *t_near = tmin;
    if (t_far) *t_far = tmax;

    return 1;
}
