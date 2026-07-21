/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include <unistd.h>
#include "connection.h"
#include "pkt.h"

void conn::clear(Connection& conn) {
    conn.fd_ = -1;
    conn.id_ = 0;
    memset(&conn.keys_.remote_auth_, 0, KEY_SIZE);
    memset(&conn.keys_.remote_session_, 0, KEY_SIZE);
    memset(&conn.keys_.session_, 0, sizeof(KeyPair));
    memset(&conn.keys_.rx_, 0, KEY_SIZE);
    memset(&conn.keys_.tx_, 0, KEY_SIZE);
    conn.events_ = 0;
    conn.failure_count_ = 0;
    conn.status_ = Status::Dead;

    conn.rpkt_.buff_->len = 0;
    memset(conn.rpkt_.buff_->b, 0, conn.rpkt_.buff_->cap);
}

bool conn::enable_epollout(Connection& conn, Actor* a) {
    EpollEvent eev {.events = conn.events_ |= EPOLLOUT };
    eev.data.fd = conn.fd_;
    return ctl_epoll(a, &eev,EPOLL_CTL_MOD) == -1;
}

bool conn::disable_epollout(Connection& conn, Actor* a) {
    EpollEvent eev {.events = conn.events_ &= ~EPOLLOUT };
    eev.data.fd = conn.fd_;
    return ctl_epoll(a, &eev, EPOLL_CTL_MOD) == -1;
}

Error conn::read_(
    Connection& conn,
    BufferStore* buffs
) {
    while (true) {

        // READING PREFIX
        if (conn.rpkt_.prefix_cursor_ < Packet::PREFIX_LEN) {
            size_t n = read(conn.fd_, conn.rpkt_.get_prefix_cursor(), Packet::PREFIX_LEN);
            if (n <= 0) break;
            conn.rpkt_.prefix_cursor_ += n;

            if (conn.rpkt_.prefix_cursor_ == Packet::PREFIX_LEN) {
                uint64_t len;
                conn.rpkt_.get_len(&len) ;

                if (len > Packet::MAX_LEN) {
                    conn.rpkt_.wipe();
                    conn.status_ = conn::Status::Failed;
                    return { -1, conn.id_, E_OVERSIZED, conn.keys_.remote_auth_,  "read_() :: MAX Pkt size " };
                }
            } else {
                break;
            }
        }


        // READING BODY
        uint64_t target;
        conn.rpkt_.get_len(&target) ;

        if (target > 0 && !conn.rpkt_.buff_) {
            conn.rpkt_.buff_ = grab_buff(buffs, target);
        }
        if (conn.rpkt_.body_cursor_ > Packet::PREFIX_LEN) {
            ssize_t n = read(conn.fd_, conn.rpkt_.get_body_cursor(), target - conn.rpkt_.body_cursor_);
            if (n <= 0) break;
            conn.rpkt_.body_cursor_ += n;
            if (conn.rpkt_.body_cursor_ < target) break;
        }

        // DECRYPT BODY
        int r = conn.rpkt_.decrypt_body(conn.keys_.rx_);
        if (r != 0) {
            conn.rpkt_.wipe();
            conn.failure_count_++;
            put_buff(buffs, conn.rpkt_.buff_);
            return { r, conn.id_, E_MALFORMED, conn.keys_.remote_auth_, "read_() :: decrypt" };
        }

        return ESUCCESS;
    }
    return ESUCCESS;
}


int conn::write_(Connection& conn, P2P& man) {
    if (!conn.wpkt_.buff_ && conn.pending_ids_.size() > 0) {
        int mid = conn.pending_ids_.front();
        const Msg& m = man.messenger_.get_msg(mid);

        conn.wpkt_.set_code(m.code);
        conn.wpkt_.set_too(m.too);

        conn.wpkt_.set_key(man.keys_.pub);
        conn.wpkt_.set_version(conn.version_);

        if (m.data->len > 0) {
            conn.wpkt_.set_len(m.data->len);
            conn.wpkt_.buff_ = grab_buff(man.buffers_, m.data->len);
            memcpy(conn.wpkt_.buff_->b, m.data->b, m.data->len);
        }
    }

    uint8_t* cursor;
    uint64_t total;
    uint64_t* offset;

    if (conn.wpkt_.prefix_cursor_ < Packet::PREFIX_LEN) {
        total = Packet::PREFIX_LEN;
        offset = &conn.wpkt_.prefix_cursor_;
        cursor = conn.wpkt_.get_prefix_cursor();
    } else {
        conn.wpkt_.get_len(&total);
        offset = &conn.wpkt_.body_cursor_;
        cursor = conn.wpkt_.get_body_cursor();
    }

    uint64_t written = write(conn.fd_, cursor, total - *offset);
    if (written < 0 && (errno = EAGAIN || errno == EWOULDBLOCK)) {
        return -1;
    } else if (written < 0) {
        conn.failure_count_++;
    }
    *offset += written;

    if (*offset == total) {
        put_buff(man.buffers_, conn.wpkt_.buff_);
        conn.wpkt_.buff_ = NULL;
        int mid = conn.pending_ids_.front();
        conn.pending_ids_.erase(conn.pending_ids_.begin());

        conn.wpkt_.wipe();

        if (conn.pending_ids_.size() == 0) 
            disable_epollout(conn, man.chans_);

        return mid;
    }

    return -1;
}
