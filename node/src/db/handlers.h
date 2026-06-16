#pragma once
#include "db/db.h"

enum class DBPaths : int {
    USER_INSERT,
    USER_DELETE,

    CARD_RECENT,

    PERM_INSERT,
    PERM_DELETE,

    VOUCHER_INSERT,
    NEW_BLOB,
};

static const char* STMT_USER_INSERT =  
    "INSERT INTO users (id) VALUES(?);";
void user_insert(db::Server& db, Error& e, const Msg* msg);

static const char* STMT_USER_DELETE =  
    "DELETE FROM users WHERE id = (?);";
void user_delete(db::Server& db, Error& e, const Msg* msg);

static const char* STMT_CARD_RECENT =
    "SELECT * "
    "FROM id_cards "
    "WHERE user_id = (?) "
    "ORDER BY created_at DESC "
    "LIMIT 30;";
void card_recent(db::Server& db, Error& e, const Msg* msg);

const std::vector<std::tuple<const char*, Stmts>> STATEMENTS {
    { STMT_USER_INSERT, Stmts::UserInsert },
    { STMT_USER_DELETE, Stmts::UserDelete },
    { STMT_CARD_RECENT, Stmts::CardRecent },
};
