/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "ecs/bvh/bvh.h"

bool bvh_insert(BVH *bvh, EntityID entity, AABB bounds) {
    uint32_t g_id = entity_global(entity);

    if (bvh->entity_to_leaf[g_id] != BVH_NULL) return false;

    BVHNodeID leaf = _bvh_alloc_node(bvh);

    if (leaf == BVH_NULL) return false;

    BVHNode *node = &bvh->nodes[leaf];

    node->bounds = aabb_fatten(bounds, 0.1f);
    node->parent = BVH_NULL;
    node->left   = BVH_NULL;
    node->right  = BVH_NULL;
    node->entity = entity;

    bvh->entity_to_leaf[g_id] = leaf;

    _bvh_insert_leaf(bvh, leaf);

    return true;
}


bool bvh_remove(BVH *bvh, EntityID entity)
{
    uint32_t g_id = entity_global(entity);

    BVHNodeID leaf =
        bvh->entity_to_leaf[g_id];

    if (leaf == BVH_NULL)
        return false;

    _bvh_remove_leaf(bvh, leaf);

    _bvh_free_node(bvh, leaf);

    bvh->entity_to_leaf[g_id] = BVH_NULL;

    return true;
}

bool bvh_update(BVH *bvh, EntityID entity, AABB new_bounds)
{
    uint32_t g_id = entity_global(entity);

    BVHNodeID leaf = bvh->entity_to_leaf[g_id];

    if (leaf == BVH_NULL)
        return false;

    BVHNode *node = &bvh->nodes[leaf];

    // Still inside the fat AABB.
    if (aabb_contains(node->bounds, new_bounds))
        return true;

    // It escaped the fat bounds.
    _bvh_remove_leaf(bvh, leaf);

    node->bounds = aabb_fatten(new_bounds, 0.1f);

    _bvh_insert_leaf(bvh, leaf);

    return true;
}
