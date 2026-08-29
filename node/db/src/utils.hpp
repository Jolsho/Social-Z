/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include <cstdint>
#include <string_view>

enum Stmts : uint8_t {
    UserInsert,
    UserDelete,
    CardRecent,

    Count
};


inline constexpr std::string_view schema_sql = R"sql(

-- schema.sql

PRAGMA foreign_keys = ON;

--------------------------------------------------
-- USERS
--------------------------------------------------

CREATE TABLE users (
    id BLOB PRIMARY KEY,          -- 32-byte binary ID
    created_at INTEGER NOT NULL DEFAULT (unixepoch())
);

--------------------------------------------------
-- ID CARDS
--------------------------------------------------

CREATE TABLE id_cards (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    user_id BLOB NOT NULL,

    hash BLOB NOT NULL,

    created_at INTEGER NOT NULL DEFAULT (unixepoch()),

    FOREIGN KEY (user_id)
        REFERENCES users(id)
        ON DELETE CASCADE
);

-- Fast ordering by creation time
CREATE INDEX idx_id_cards_issued_at
ON id_cards(issued_at DESC);

-- Fast lookup by user
CREATE INDEX idx_id_cards_user_id
ON id_cards(user_id);

--------------------------------------------------
-- NOTIFICATIONS
--------------------------------------------------

CREATE TABLE notifications (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    user_id BLOB NOT NULL,
    hash TEXT NOT NULL,
    is_read INTEGER NOT NULL DEFAULT 0,

    created_at INTEGER NOT NULL DEFAULT (unixepoch()),

    FOREIGN KEY (user_id)
        REFERENCES users(id)
        ON DELETE CASCADE
);

-- Fast chronological queries
CREATE INDEX idx_notifications_created_at
ON notifications(created_at DESC);

-- Fast user notification lookup
CREATE INDEX idx_notifications_user_id
ON notifications(user_id);

)sql";
