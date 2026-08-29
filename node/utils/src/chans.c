/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_node/utils/chans.h"
#include <memory.h>
#include <stdlib.h>

Channel* new_channel(
    size_t msg_cap, Actors parent, 
    const ChannelSizes* sizes,
    const ChannelSizes* budgets
) 
{ 
    Channel* c = (Channel*)malloc(sizeof(Channel));

    c->q_[PRIORITY_CRIT] = *new_queue(sizeof(size_t), sizes->critical, parent);
    c->q_[PRIORITY_CONT] = *new_queue(sizeof(size_t), sizes->control, parent);
    c->q_[PRIORITY_WORK] = *new_queue(sizeof(size_t), sizes->work, parent);
    c->q_[PRIORITY_TELE] = *new_queue(sizeof(size_t), sizes->telemetry, parent);

    c->free_ =  *new_queue(sizeof(size_t), msg_cap, parent);
    c->in_use_ =  *new_queue(sizeof(size_t), msg_cap, parent);

    c->msgs_ = (Msg*)malloc(sizeof(Msg) * msg_cap);
    c->budgets_[PRIORITY_CRIT] = budgets->critical;
    c->budgets_[PRIORITY_CONT] = budgets->control;
    c->budgets_[PRIORITY_WORK] = budgets->work;
    c->budgets_[PRIORITY_TELE] = budgets->telemetry;

    return c;
}

void delete_channel(Channel* c) {
    free(c->msgs_);
    free(c);
}

bool has_space(Channel* c, Priority p) { 
    return q_has_space(&c->q_[p]); 
}

void change_budget(Channel* c, size_t idx, size_t bud) {
    c->budgets_[idx] = bud;
}

QueueStats stats(Channel* c, Priority p) {
    QueueStats s = {
        .accepted = c->accepted_[p],
        .dropped = c->dropped_[p],
        .delta_accepted = c->accepted_[p] - c->accepted_last_log_[p],
        .delta_dropped = c->dropped_[p] - c->dropped_last_log_[p]
    };
    s.drop_rate = (float)(s.delta_accepted + s.delta_dropped) / s.delta_dropped;

    c->accepted_last_log_[p] = c->accepted_[p];
    c->dropped_last_log_[p] = c->dropped_[p];

    return s;
}
ChanSetStats snapshot(Channel* c) {
    ChanSetStats v = {
        .critical = stats(c, PRIORITY_CRIT),
        .control = stats(c, PRIORITY_CONT),
        .work = stats(c, PRIORITY_WORK),
        .telemetry = stats(c, PRIORITY_TELE)
    };
    return v;
}



void poll(Channel* c, MsgBuffer* msgs) {

    memmove(msgs->msgs_, (msgs->msgs_ + msgs->cursor_), msgs->size_ - msgs->cursor_);
    msgs->size_ -= msgs->cursor_;
    msgs->cursor_ = 0;

    size_t free_slots = msgs->cap_ - msgs->size_;

    size_t laps = 0;

    size_t* cc = q_front(&c->in_use_);
    while (laps < PRIORITY_COUNT && 0 < free_slots) {

        size_t min = c->budgets_[c->current_];
        if (min > free_slots) min = free_slots;

        size_t loaded = q_take_elements(&c->q_[c->current_], &c->in_use_, min);
        free_slots -= loaded;

        for (;0 < loaded && cc != NULL; loaded--) {
            *(msgs->msgs_ + msgs->size_) = &c->msgs_[*cc];
            msgs->size_++;
            cc = q_step(&c->in_use_, cc, 1);
        }

        if (c->counts_[c->current_] >= c->budgets_[c->current_]) {
            c->counts_[c->current_] = 0;
            c->current_ = ++c->current_ % PRIORITY_COUNT;
            ++laps;
        }
    }
}

void free_msgs(Channel* c, size_t size) {
    while (0 < size) {
        size_t idx = *(size_t*)q_front(&c->in_use_);
        q_pop_front(&c->in_use_);

        msg_wipe(&c->msgs_[idx]);

        *(size_t*)q_reserve(&c->free_) = idx;
        q_commit(&c->free_);
        size--;
    }
}

void get_free_msgs(Channel* c, MsgBuffer* msgs) {
    memmove(msgs->msgs_, (msgs->msgs_ + msgs->cursor_), msgs->size_ - msgs->cursor_);
    msgs->size_ -= msgs->cursor_;
    msgs->cursor_ = 0;

    size_t remaining = msgs->cap_ - msgs->size_;

    size_t* cc = q_back(&c->free_);

    while (0 < remaining && cc != NULL) {
        *(msgs->msgs_ + msgs->size_) = &c->msgs_[*cc];
        msgs->size_++;
        cc = q_step(&c->free_, cc, 1);
    }
}

void use_free_msgs(Channel* c, size_t size) {
    for (int i = 0; i < size; i++) {

        size_t idx = *(size_t*)q_back(&c->free_);
        q_pop_back(&c->free_);

        Priority p = c->msgs_[idx].priority;

        size_t* q_idx = q_reserve(&c->q_[p]);
        size_t* u_idx = q_reserve(&c->in_use_);
        if (!q_idx || !u_idx) {
            c->dropped_[p]++;
            continue;
        }

        *q_idx = idx;
        q_commit(&c->q_[p]);

        *u_idx = idx;
        q_commit(&c->in_use_);

        c->accepted_[p]++;
    }
}
