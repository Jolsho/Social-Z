#include <cstring>
#include <format>
#include <thread>
#include "server.h"
#include "handlers.h"
#include "utils/path.h"
#include "utils/vec.h"

ActorThread* start_db(Actor* actor, DBConfig* conf) {
    ActorThread* at = new ActorThread{.r = 0};
    DB* db = new DB(actor, conf);
    at->t = (void*)new std::thread([&] {
        db->poll_loop();
        delete db;
    });
    return at;
}

DB::DB(
    Actor* actor, 
    DBConfig* conf
) : 
    chans_(actor), 
    db_(cpy_apnd(PATHS.data_dir, {"/db/lmdb"}).c_str(), conf->map_size),
    buffers_(BufferCaps{})
{

    
    int rc = sqlite3_open(cpy_apnd(PATHS.data_dir, {"/db/sql"}).c_str(), &sql_);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open DB: %s\n", sqlite3_errmsg(sql_));
        sqlite3_close(sql_);
    }
    
    char* err;
    rc = sqlite3_exec(sql_, schema_sql.data(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to execute DB schema: %s\n", sqlite3_errmsg(sql_));
        sqlite3_free(err);
        sqlite3_close(sql_);
    }

    for (const auto& [s,st]: STATEMENTS) {
        sqlite3_prepare_v2(sql_, s, -1, &stmts_[st], nullptr);
    }

    static constexpr time_t LOG_FLUSH_INTERVAL = 500; // ms
    logr_ = new LogAccumulator{"DB", LOG_FLUSH_INTERVAL, buffers_, ACTOR_DB };

    handlers_ = {
        Handler { .path   = "user_insert", .handle = user_insert },
        Handler { .path   = "user_delete", .handle = user_delete },
        Handler { .path   = "card_recent", .handle = card_recent },
    };
}

void DB::shutdown() {
    sqlite3_close(sql_);
    for (auto& stmt: stmts_)
        sqlite3_finalize(stmt);
}

void DB::handle_msg(Error& e, Msg* msg) {

    if (!msg->data) {
        e.msg = "Request has no body.";
        e.code = E_MALFORMED;
        return;
    }


    uint64_t path_len = vec_read<uint64_t>(msg->data);
    std::string path;
    path.reserve(path_len);
    vec_read(msg->data, (unsigned char*)path.data(), path_len);

    for (auto& h: handlers_) {
        if (path == h.path) {
            h.handle(*this, e, msg);
            return;
        }
    }
    e.msg = std::format("Invalid Path: {}", path);
    e.code = E_MALFORMED;
}

void DB::handle_error(Error& e) {
}
