#pragma once
#include "msg.h"
#include <ctime>

struct ConnNode {
    ConnNode*   prev    = nullptr;
    ConnNode*   next    = nullptr;
    ConnID      key     = 0;
    time_t      expires = time(nullptr);
};

template <std::size_t T, time_t E>
class ConnLRU {
    size_t count = 0;
    size_t cap   = T;
    ConnNode* head = nullptr;
    ConnNode* tail = nullptr;

public:

    inline const ConnNode* get_tail() const { return tail; }
    inline const ConnNode* get_head() const { return head; }


    inline size_t size() const {  return count; }

    int use(ConnNode* n) {

        n->expires = time(nullptr) + E;

        if (n == head) return -1;

        bool needs_eviction = false;
        if (!n->prev && !n->next)
            needs_eviction = ++count > cap;

        remove(n);

        // insert at front
        n->next = head;

        if (head) head->prev = n;
        head = n;

        if (!tail) tail = n;

        if (needs_eviction) return evict();
        return -1;
    }

    void remove(ConnNode* n) {
        if (n->prev)
            n->prev->next = n->next;
        if (n->next)
            n->next->prev = n->prev;

        if (n == tail)
            tail = n->prev;

        if (n == head)
            head = n->next;

        n->prev = nullptr;
        n->next = nullptr;
    }

    int evict() {
        ConnNode* n = tail;
        if (!n) return -1;

        remove(n);
        count--;
        return static_cast<int>(n->key);
    }

    std::vector<int> remove_expired() {
        std::vector<int> removed;
        time_t now = time(nullptr);
        while (tail && tail->expires < now) {
            if (removed.size() == 0) {
                removed.reserve(count / 2);
            }
            removed.push_back(evict());
        }
        return removed;
    }
};
