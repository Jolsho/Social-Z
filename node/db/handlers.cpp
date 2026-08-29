/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "server.hpp"
#include "sqlite3.h"
#include "sz/utils/error.h"

void user_insert(DB& db, Error& e, const Msg* msg) {

    auto stmt = db.get_stmt(Stmts::UserInsert);
    int i = 0;

    Key key;
    vec_read(msg->data, key.b, KEY_SIZE);
    sqlite3_bind_blob(stmt, ++i, key.b, KEY_SIZE, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        e.msg = db.get_err();
        e.code = E_INTERNAL;
    }
}

void user_delete(DB& db, Error& e, const Msg* msg) {

    auto stmt = db.get_stmt(Stmts::UserDelete);
    int i = 0;

    Key key;
    vec_read(msg->data, key.b, KEY_SIZE);
    sqlite3_bind_blob(stmt, ++i, key.b, KEY_SIZE, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        e.msg = db.get_err();
        e.code = E_INTERNAL;
    }
}

void card_recent(DB& db_, Error& e, const Msg* msg) {

    static constexpr size_t LEN = KEY_SIZE + sizeof(uint64_t);
    if (vec_remaining(msg->data) < LEN) {
        e.code = E_UNDERSIZED;
        e.msg = "card_recent() :: msg.data is below expected size.";
        return;
    }

    auto stmt = db_.get_stmt(Stmts::CardRecent);
    int i = 0;

    Key key;
    vec_read(msg->data, key.b, KEY_SIZE);

    uint64_t offset;
    vec_read(msg->data, &offset, sizeof(uint64_t));
    sqlite3_bind_blob(stmt, ++i, key.b, KEY_SIZE, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, ++i, offset);


    Vec* r_buff = db_.get_buffer(sizeof(Card) * 30);

    uint8_t* len_p = r_buff->c;
    size_t len = 0;
    vec_write(r_buff, &len, sizeof(size_t));

    Card card;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        card.id = sqlite3_column_int(stmt, 0);

        const void* hash = sqlite3_column_blob(stmt, 2);
        memcpy(&card.hash, hash, HASH_SIZE);

        card.created_at = sqlite3_column_int(stmt, 3);

        vec_write(r_buff, &card.id, sizeof(int));
        vec_write(r_buff, card.hash.b, HASH_SIZE);
        vec_write(r_buff, &card.created_at, sizeof(int));

        len++;
    }

    memcpy(len_p, &len, sizeof(size_t));

    Msg* rmsg = consume_msg(db_.free_out_msgs_);
    rmsg->id = msg->id;
    rmsg->code = msg->code;
    rmsg->data = r_buff;
    vec_read(msg->data, &rmsg->id, sizeof(ConnID));
}
