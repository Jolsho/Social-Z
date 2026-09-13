/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "ecs/bvh/bvh.h"
#include <math.h>

EntityID bvh_raycast(const BVH* bvh, Ray ray) {

    float closes_t = INFINITY;
    EntityID id = ENTITY_ID_INVALID;

    uint32_t stack[64];
    uint32_t stack_size = 0;

    BVHNode* node;
    uint32_t index;
    float t_near, t_far;

    stack[stack_size++] = bvh->root;

    while (stack_size) {
        index = stack[--stack_size];

        node = &bvh->nodes[index];


        if (!aabb_ray_intersect(node->bounds, ray, &t_near, &t_far))
            continue;

        /*
         * Already have something closer.
         * This entire node cannot produce a better result.
         */
        if (t_near > closes_t) continue;

        if (_bvh_node_is_leaf(node)) {
            closes_t = t_near;
            id = node->entity;
            continue;
        }

        stack[stack_size++] = node->left;
        stack[stack_size++] = node->right;
    }

    return id;
}

void bvh_query_aabb(
    BVH *bvh, AABB query,
    BVHQueryCallback callback,
    void *ctx
)
{
    if (bvh->root == BVH_NULL)
        return;

    BVHNodeID stack[64];
    uint32_t stack_size = 0;

    stack[stack_size++] = bvh->root;

    while (stack_size) {
        BVHNodeID id = stack[--stack_size];
        BVHNode *node = &bvh->nodes[id];

        if (!aabb_overlap(node->bounds, query))
            continue;

        if (_bvh_node_is_leaf(node)) {

            if (!callback(node->entity, ctx))
                return;

            continue;
        }

        stack[stack_size++] = node->left;
        stack[stack_size++] = node->right;
    }
}

void bvh_query_sphere(
    BVH *bvh,
    Vec3 center,
    float radius,
    BVHQueryCallback callback,
    void *ctx
)
{
    if (bvh->root == BVH_NULL)
        return;

    BVHNodeID stack[64];
    uint32_t stack_size = 0;

    stack[stack_size++] = bvh->root;

    while (stack_size) {
        BVHNodeID id = stack[--stack_size];
        BVHNode *node = &bvh->nodes[id];

        if (!sphere_aabb_overlap(center, radius, node->bounds))
            continue;

        if (_bvh_node_is_leaf(node)) {
            if (!callback(node->entity, ctx))
                return;

            continue;
        }

        stack[stack_size++] = node->left;
        stack[stack_size++] = node->right;
    }
}
void bvh_query_frustum(
    BVH *bvh,
    Frustum frustum,
    BVHQueryCallback callback,
    void *ctx
)
{
    if (bvh->root == BVH_NULL)
        return;

    BVHNodeID stack[64];
    uint32_t stack_size = 0;

    stack[stack_size++] = bvh->root;

    while (stack_size) {
        BVHNodeID id = stack[--stack_size];
        BVHNode *node = &bvh->nodes[id];

        if (!aabb_inside_frustum(node->bounds, frustum))
            continue;

        if (_bvh_node_is_leaf(node)) {
            if (!callback(node->entity, ctx))
                return;

            continue;
        }

        stack[stack_size++] = node->left;
        stack[stack_size++] = node->right;
    }
}

