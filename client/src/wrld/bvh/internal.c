/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "wrld/bvh/bvh.h"
#include <string.h>

BVHNodeID _bvh_alloc_node(BVH *bvh) {
    if (bvh->free_list != BVH_NULL) {
        BVHNodeID id = bvh->free_list;

        bvh->free_list = bvh->nodes[id].parent;

        return id;
    }

    if (bvh->node_count >= bvh->node_capacity) {
        return BVH_NULL;
    }

    return bvh->node_count++;
}

void _bvh_free_node(BVH *bvh, BVHNodeID id) {
    bvh->nodes[id].parent = bvh->free_list;
    bvh->free_list = id;
}

float _bvh_insertion_cost(AABB current, AABB leaf) {
    AABB combined = aabb_union(current, leaf);

    return aabb_surface_area(combined)
         - aabb_surface_area(current);
}

BVHNodeID _bvh_find_best_sibling(BVH *bvh, AABB leaf_bounds) {
    BVHNodeID index = bvh->root;
    BVHNode *node = &bvh->nodes[index];

    while (!_bvh_node_is_leaf(node)) {

        BVHNodeID left  = node->left;
        BVHNodeID right = node->right;

        float cost_left =
            _bvh_insertion_cost(
                bvh->nodes[left].bounds,
                leaf_bounds
            );

        float cost_right =
            _bvh_insertion_cost(
                bvh->nodes[right].bounds,
                leaf_bounds
            );

        index = cost_left < cost_right ? left : right;
        node = &bvh->nodes[index];
    }

    return index;
}

void _bvh_insert_leaf(BVH *bvh, BVHNodeID leaf) {
    // Empty tree.
    if (bvh->root == BVH_NULL) {
        bvh->root = leaf;
        bvh->nodes[leaf].parent = BVH_NULL;
        return;
    }

    BVHNodeID sibling =
        _bvh_find_best_sibling(
            bvh,
            bvh->nodes[leaf].bounds
        );

    BVHNodeID old_parent =
        bvh->nodes[sibling].parent;

    BVHNodeID new_parent =
        _bvh_alloc_node(bvh);

    BVHNode *parent = &bvh->nodes[new_parent];

    parent->parent = old_parent;
    parent->left   = sibling;
    parent->right  = leaf;
    memset(&parent->entity, 0, sizeof(EntityID));

    parent->bounds =
        aabb_union(
            bvh->nodes[sibling].bounds,
            bvh->nodes[leaf].bounds
        );

    bvh->nodes[sibling].parent = new_parent;
    bvh->nodes[leaf].parent = new_parent;

    if (old_parent == BVH_NULL) {
        // Sibling was root.
        bvh->root = new_parent;
    } else {
        BVHNode *p = &bvh->nodes[old_parent];

        if (p->left == sibling)
            p->left = new_parent;
        else
            p->right = new_parent;
    }

    // Walk upward and refit bounds.
    BVHNodeID index = new_parent;

    while (index != BVH_NULL) {
        BVHNode *node = &bvh->nodes[index];

        if (node->left != BVH_NULL) {
            node->bounds =
                aabb_union(
                    bvh->nodes[node->left].bounds,
                    bvh->nodes[node->right].bounds
                );
        }

        index = node->parent;
    }
}


void _bvh_remove_leaf( BVH *bvh, BVHNodeID leaf) {

    // Leaf is root.
    if (leaf == bvh->root) {
        bvh->root = BVH_NULL;
        return;
    }

    BVHNodeID parent =
        bvh->nodes[leaf].parent;

    BVHNodeID grandparent =
        bvh->nodes[parent].parent;

    BVHNodeID sibling;

    if (bvh->nodes[parent].left == leaf)
        sibling = bvh->nodes[parent].right;
    else
        sibling = bvh->nodes[parent].left;

    if (grandparent == BVH_NULL) {
        // Parent was root.
        bvh->root = sibling;
        bvh->nodes[sibling].parent = BVH_NULL;
    } else {
        // Replace parent with sibling.
        BVHNode *gp = &bvh->nodes[grandparent];

        if (gp->left == parent)
            gp->left = sibling;
        else
            gp->right = sibling;

        bvh->nodes[sibling].parent = grandparent;

        // Refit all ancestors.
        BVHNodeID index = grandparent;

        while (index != BVH_NULL) {
            BVHNode *node = &bvh->nodes[index];

            node->bounds =
                aabb_union(
                    bvh->nodes[node->left].bounds,
                    bvh->nodes[node->right].bounds
                );

            index = node->parent;
        }
    }

    _bvh_free_node(bvh, parent);
}
