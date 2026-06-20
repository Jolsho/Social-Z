#include "db.h"
#include "bindings.h"
#include "utils/error.h"
#include "utils/vec.h"
#include <cstdint>
#include <cstring>

void user_insert(db::Server& db, Error& e, const Msg* msg) {

    auto stmt = db.get_stmt(Stmts::UserInsert);
    int i = 0;

    Key key = vec_read<Key>(msg->data);
    sqlite3_bind_blob(stmt, ++i, key.data(), KEY_SIZE, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        e.msg = db.get_err();
        e.code = E_INTERNAL;
    }
}

void user_delete(db::Server& db, Error& e, const Msg* msg) {

    auto stmt = db.get_stmt(Stmts::UserDelete);
    int i = 0;

    Key key = vec_read<Key>(msg->data);
    sqlite3_bind_blob(stmt, ++i, key.data(), KEY_SIZE, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        e.msg = db.get_err();
        e.code = E_INTERNAL;
    }
}

void card_recent(db::Server& db_, Error& e, const Msg* msg) {

    static constexpr size_t LEN = KEY_SIZE + sizeof(uint64_t);
    if (vec_remaining(msg->data) < LEN) {
        e.code = E_UNDERSIZED;
        e.msg = "card_recent() :: msg.data is below expected size.";
        return;
    }

    auto stmt = db_.get_stmt(Stmts::CardRecent);
    int i = 0;

    Key key = vec_read<Key>(msg->data);
    uint64_t offset = vec_read<uint64_t>(msg->data);
    sqlite3_bind_blob(stmt, ++i, key.data(), KEY_SIZE, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, ++i, offset);


    Vec* r_buff = db_.get_buffer(sizeof(Card) * 30);

    Card card;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        card.id = sqlite3_column_int(stmt, 0);

        const void* hash = sqlite3_column_blob(stmt, 2);
        memcpy(&card.hash, hash, HASH_SIZE);

        card.created_at = sqlite3_column_int(stmt, 3);


        vec_write(r_buff, card);
    }


    Msg* rmsg ;//= use_msg(db_.get_buffer);
    rmsg->id = msg->id;
    rmsg->code = msg->code;
    rmsg->data = r_buff;
    rmsg->id = vec_read<ConnID>(msg->data);
}
