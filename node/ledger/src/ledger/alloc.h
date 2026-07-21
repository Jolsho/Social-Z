/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */


#pragma once
#include "trie/node.h"
#include "ledger/db.h"
#include "utils/lru.h"
#include "utils/result.h"

struct Gadgets;

class NodeAllocator {
public:
    NodeAllocator(
        std::string path,
        size_t cache_size,
        size_t map_size
    );
    ~NodeAllocator();

    std::shared_mutex mux_;
    LRUCache<NodeId, std::shared_ptr<Node>, NodeIdHash> cache_;
    BulletDB db_;
    std::shared_ptr<Gadgets> gadgets_;

    void set_gadgets(std::shared_ptr<Gadgets> gadgets);

    Result<std::shared_ptr<Node>, int> load_node(
        const NodeId* id, 
        bool needs_lock = false
    );
    Result<std::shared_ptr<Node>, int> delete_node(
        const NodeId* id,
        bool needs_lock = false
    );
    int recache(const NodeId *old_id, const NodeId *new_id, bool needs_lock = false);
    Node_ptr cache_node(std::shared_ptr<Node> node, bool needs_lock = false);
    void persist_node(Node* node);
};
