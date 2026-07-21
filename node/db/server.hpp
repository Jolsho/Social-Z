/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz/utils/buffers.h"
#include "sz/db.h"
#include "sz/utils/error.h"
#include "sz/utils/accumulator.h"
#include "sz/lmdb.h"
#include "sqlite3.h"
#include <functional>
#include <vector>
#include <string>
#include "utils.hpp"

class DB {
    LogAccumulator*             logr_;

    // MSGING
    int                         epoll_fd_;
    BufferStore*                 buffers_;

    // Stores
    LMDB*                       db_;
    sqlite3*                    sql_;

    std::array<sqlite3_stmt*, Stmts::Count> stmts_;


    struct Handler {
        std::string path;
        std::function<void(DB&, Error&, const Msg*)> handle;
    };
    std::vector<Handler>    handlers_;

public:
    Actor*              chans_;

    MsgBuffer*  free_out_msgs_;
    MsgBuffer*  in_msgs_;

    DB(Actor *chans, DBConfig* conf);
    void poll_loop();

    void handle_msg(Error& e, Msg* msg);
    void handle_error(Error& e);

    void shutdown();

    inline sqlite3_stmt* get_stmt(Stmts st) { 
        sqlite3_stmt* stm = stmts_[st];
        sqlite3_reset(stm);
        sqlite3_clear_bindings(stm);
        return stm; 
    }
    inline const char* get_err() { return sqlite3_errmsg(sql_); }

    inline Vec* get_buffer(size_t sz) { return grab_buff(buffers_, sz); }
};

