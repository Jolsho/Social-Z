/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include <list>
#include <unordered_map>
#include <optional>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : cap(capacity) {}

    Value* get(const Key& key) {
        auto it = map.find(key);
        if (it == map.end())
            return nullptr;

        order.splice(order.begin(), order, it->second);
        return &it->second->second;
    }

    std::optional<std::tuple<Key, Value>> put(const Key& key, Value value) {
        auto it = map.find(key);
        if (it != map.end()) {
            it->second->second = value;
            order.splice(order.begin(), order, it->second);
            return std::nullopt;
        }

        order.emplace_front(key, value);
        map.emplace(key, order.begin());

        if (map.size() > cap) {
            auto last = std::prev(order.end());
            map.erase(last->first);
            auto evicted = std::tuple<Key, Value>{last->first, last->second};
            order.pop_back();
            return evicted;
        }

        return std::nullopt;
    }

    Value remove(const Key& key) {
        auto it = map.find(key);
        if (it == map.end())
            return Value{};

        Value val = it->second->second;
        order.erase(it->second);
        map.erase(it);
        return val;
    }

private:
    size_t cap;
    std::list<std::pair<Key, Value>> order;
    std::unordered_map<Key,
        typename std::list<std::pair<Key, Value>>::iterator,
        Hash> map;
};

