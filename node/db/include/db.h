#pragma once
#include "utils/buffers.h"
#include "config.h"
#include "utils/error.h"
#include "utils/accumulator.h"
#include "db_iface.h"
#include "sqlite3.h"
#include <functional>
#include <vector>
#include "utils.h"

namespace db {


class Server {
    LogAccumulator*             logr_;

    // MSGING
    int                         epoll_fd_;
    BufferStore                 buffers_;

    // Stores
    LMDB                        db_;
    sqlite3*                    sql_;

    std::array<sqlite3_stmt*, Stmts::Count> stmts_;


    struct Handler {
        std::string path;
        std::function<void(Server&, Error&, const Msg*)> handle;
    };
    std::vector<Handler>    handlers_;

public:
    Actor*               chans_;

    MsgBuffer*  out_msgs_;
    MsgBuffer*  in_msgs_;

    Server(Actor *chans, DBConfig& conf);
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

    inline Vec* get_buffer(size_t sz) { return buffers_.grab(sz); }
};
}

