/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "sz_common/pqueue.h"
#include "wrld/bvh/bvh.h"

typedef struct {
    BVHNodeID   id;
    float       t;
} Hit;

int compare_hit(void* n1, void* n2) {
    int dif = ((Hit*)n1)->t - ((Hit*)n2)->t;
    if (dif > 0) return 1;
    else if (dif < 0) return -1;
    return 0;
}

void bvh_raycast(const BVH* bvh, Ray ray, EntityID* hits, size_t* hit_cnt) {
    size_t hit_cap = *hit_cnt;
    *hit_cnt = 0;

    PriorityQueue queue;
    pq_init(&queue, sizeof(Hit), 64, compare_hit);

    float t_far;
    Hit curr, child;
    curr.id = bvh->root;

    pq_push(&queue, &curr);

    BVHNode* node;
    while (!pq_is_empty(&queue) && (hit_cap - 1) > *hit_cnt) {

        pq_pop(&queue, &curr);

        node = &bvh->nodes[curr.id];
        if (_bvh_node_is_leaf(node)) {
            hits[*hit_cnt] = node->entity;
            (*hit_cnt)++;
            continue;
        }

        child.id = bvh->nodes[curr.id].left;
        if (aabb_ray_intersect(bvh->nodes[child.id].bounds, ray, &child.t, &t_far)) {
            pq_push(&queue, &child);
        }

        child.id = bvh->nodes[curr.id].right;
        if (aabb_ray_intersect(bvh->nodes[child.id].bounds, ray, &child.t, &t_far)) {
            pq_push(&queue, &child);
        }
    }

    return;
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

