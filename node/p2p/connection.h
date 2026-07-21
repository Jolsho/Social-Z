/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "manager.h"
#include <sys/epoll.h>

namespace conn {

static constexpr size_t MAX_PENDING_OUT = 32;
int write_(Connection& c, P2P& netman);
Error read_(Connection& c, BufferStore* buffs);

inline bool is_epollout_enabled(Connection& c) { 
    return (c.events_ & EPOLLOUT) != 0; 
}

bool disable_epollout(Connection& c, Actor* a);
bool enable_epollout(Connection& c, Actor* a);

Error syn(Connection& conn, P2P& man);
Error syn_ack(Connection& conn, P2P& man);
Error ack(Connection& conn, P2P& man);
void clear(Connection& c);

}
