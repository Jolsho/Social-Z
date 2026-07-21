/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz/api/actor.h"
#include "sz/api/msgT.h"
#include "sz/utils/vec.h"
#include <cstring>
#include <optional>
#include <vector>

class Messenger {
    std::vector<Msg>                    msgs;
    std::vector<std::vector<Key>>       recipients;
    size_t                              wave;

    int                     next_id_;
    std::vector<int>        free_ids_;

public:

    Messenger(size_t wave, size_t cap) : wave(wave) {
        msgs.reserve(cap);
    }

    int new_msg(Msg* msg) {

        int i;
        if (!free_ids_.empty()) {
            i = free_ids_.back();
            free_ids_.pop_back();
        } else {
            i = next_id_++;
        }

        if (!msg) return i;

        PktCode code;
        vec_read(msg->data, &code, sizeof(PktCode));

        Actors too;
        vec_read(msg->data, &too, sizeof(Actors));
        

        uint64_t total_recipients;
        vec_read(msg->data, &total_recipients, sizeof(uint64_t));

        recipients[i].reserve(total_recipients);

        for (auto i { 0 }; i < total_recipients; i++) {
            auto& r = recipients[i].emplace_back();
            vec_read(msg->data, r.b, KEY_SIZE);
        };

        vec_shift_remaining(msg->data);
        msgs[i] = *msg;
        msgs[i].code = code;
        msgs[i].too = too;

        msg->data = NULL;

        return i;
    }

    inline const Msg& get_msg(int mid) { 
        return msgs[mid]; 
    }
    inline Msg& get_mut_msg(int mid) { 
        return msgs[mid]; 
    }

    std::optional<std::tuple<Key, Msg>> next(int mid) {

        Msg m = msgs[mid];

        if (recipients[mid].size() == 0) {
            return std::nullopt;
        }

        Key next = recipients[mid].back();
        recipients[mid].pop_back();

        return {{next, m}};
    }

    std::optional<Msg> remove(int mid) {
        recipients[mid].clear();
        if (msgs[mid].is_wiped) return std::nullopt;
        free_ids_.push_back(mid);
        Msg m = msgs[mid];
        msg_wipe(&msgs[mid]);
        return m;
    }
};
