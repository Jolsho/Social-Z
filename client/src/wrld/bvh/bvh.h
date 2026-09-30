/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "wrld/id.h"
#include "math/aabb.h"
#include "math/frustrum.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BVH_NULL UINT32_MAX

typedef uint32_t BVHNodeID;

typedef struct {
    AABB    bounds;

    BVHNodeID parent;
    BVHNodeID left;
    BVHNodeID right;

    EntityID entity;
} BVHNode;


typedef struct {
    BVHNode*    nodes;
    uint32_t    node_count;
    uint32_t    node_capacity;

    // EntityID -> leaf node.
    BVHNodeID*  entity_to_leaf;

    uint32_t    root;
    BVHNodeID   free_list;
} BVH;

int bvh_init(BVH*);

bool bvh_insert(BVH*, EntityID, AABB, float margin);
bool bvh_remove(BVH*, EntityID);
bool bvh_update(BVH*, EntityID, AABB);

void bvh_raycast(const BVH* bvh, Ray ray, EntityID* hits, size_t* hit_cnt);


// TODO -> these individually used callbacks arent right.
//  This should be like filling a queue of entityIDs.
//  Or atleast make a variant that does that.
//  This way we can use batches and cpu cache to process faster
//      Like if you are just destroying a post with children.
//      And all of them have null destroy operator.
//      You would like that to happen very quickly.
//      Also it helps that you will know the quantity of entities within a post.
//      So you can actually make the queue match the children count.


typedef bool (*BVHQueryCallback)(EntityID, void*);
void bvh_query_aabb(BVH*, AABB, BVHQueryCallback, void*);
void bvh_query_sphere(BVH*, Vec3, float, BVHQueryCallback, void*);
void bvh_query_frustum(BVH*, Frustum, BVHQueryCallback, void*);




/*  INTERNAL */
static inline int _bvh_node_is_leaf(const BVHNode *node) {
    return node->left == BVH_NULL;
}
BVHNodeID _bvh_alloc_node(BVH *bvh);
void _bvh_free_node(BVH *bvh, BVHNodeID id);
float _bvh_insertion_cost(AABB current, AABB leaf);
BVHNodeID _bvh_find_best_sibling(BVH *bvh, AABB leaf_bounds);
void _bvh_insert_leaf(BVH *bvh, BVHNodeID leaf);
void _bvh_remove_leaf(BVH *bvh, BVHNodeID leaf);
