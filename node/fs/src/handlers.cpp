/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "manager.h"
#include "sz_common/paths.h"
#include <algorithm>

void FS::handle_internal_msgs() {
    while (Msg* msg = consume_msg(in_msgs_)) {
        if (
            !msg->is_wiped && 
            msg->data && 
            (msg->data->len > sizeof(FS_PATH))
        ) {

            // If no space just silently drop
            Error e { .id = msg->id };

            FS_PATH path;
            vec_read(msg->data, &path, sizeof(FS_PATH));
            if (msg->from == ACTOR_P2P) {
                switch (path) {
                    case FS_VOUCHER:   voucher      (msg, e);   break;
                    case FS_REDEEM:    redeem_remote(msg, e);   break;
                    case FS_REWARD:    reward       (msg, e);   break;

                    case FS_GIVE:      give_remote  (msg, e);   break;
                    case FS_SETTLE:    settle_remote(msg, e);   break;
                    case FS_ASK:       ask_remote   (msg, e);   break;
                    case FS_REVOKE:    revoke_remote(msg, e);   break;

                    default: {
                        e.msg = "FS_internal_remote() :: invalid path";
                        e.code = E_MALFORMED;
                        break;
                    }
                }
            } else {
                switch (path) {

                    case FS_REDEEM:    redeem_local (msg, e);   break;

                    case FS_GIVE:      give_local   (msg, e);   break;
                    case FS_SETTLE:    settle_local (msg, e);   break;
                    case FS_ASK:       ask_local    (msg, e);   break;
                    case FS_REVOKE:    revoke_local (msg, e);   break;

                    default: {
                        e.msg = "FS_internal_local() :: invalid path";
                        e.code = E_MALFORMED;
                        break;
                    }
                }
            }
            if (is_err(&e)) handle_err(e, (Actors)msg->from);
        }
        
        if (!msg->is_wiped) msg_wipe(msg);

        if (msg->from == ACTOR_FS) {
            put_buff(buffers_, msg->data);
        } else {
            // Return message
            Msg* m = consume_msg(free_out_msgs_);
            *m = *msg;
            m->priority = PRIORITY_WORK;
        }

    }
}

void FS::handle_outbound() {
    const int MAX_OUTS_PER_ROUND = 8;
    for (int i = 0; i < MAX_OUTS_PER_ROUND; i++) {
        Session& s = outbound_.front();

        Msg* m = consume_msg(free_out_msgs_);
        if (!m) break;

        m->priority = PRIORITY_WORK;
        uint64_t si = std::min(static_cast<uint64_t>(BUFF_SU), s.file->size - s.byte_count + SID_SZ + sizeof(uint64_t));
        m->data = grab_buff(buffers_, si);
        m->too = s.actor;

        FS_PATH p = FS_REWARD;
        vec_write(m->data, &p, sizeof(FS_PATH));
        vec_write(m->data, s.id.data(), s.id.size());

        if (lseek(s.file->fd, s.byte_count,  SEEK_SET) < 0) {
            handle_err({
                .code = E_INTERNAL,
                .key = s.voucher.to,
                .msg = "Failed to seek in file to outbound session."
            }, (Actors)s.actor);

            if (s.file->ref_count == 1) {
                close(s.file->fd);
                open_files_.erase(s.file->hash);
            }
            outbound_.pop_front();
            unconsume_msg(free_out_msgs_);

            continue;
        }


        size_t remaining = vec_remaining(m->data) - sizeof(uint64_t);

        uint8_t* len_c = m->data->c;
        uint64_t len = 0;

        int n = 0;
        while (n > 0 && 0 < remaining) {
            n = read(s.file->fd ,m->data->c, remaining);

            if (n > 0) {
                m->data->c += n;
                m->data->len += n;
                len += n;
                remaining -= n;
            } else if (n == 0) {
                break;
            }
        }

        s.byte_count += len;
        if (n < 0 || s.byte_count > s.file->size) {
            handle_err({
                .r = n,
                .code = E_INTERNAL,
                .key = s.voucher.to,
                .msg = "Failed to read chunk from file to outbound session."
            }, (Actors)s.actor);

            if (s.file->ref_count == 1) {
                close(s.file->fd);
                open_files_.erase(s.file->hash);
            }
            outbound_.pop_front();
            unconsume_msg(free_out_msgs_);
            continue;
        }

        memcpy(len_c, &len, sizeof(uint64_t));

        if (s.byte_count == s.file->size) {
            if (s.file->ref_count == 1) {
                close(s.file->fd);
                open_files_.erase(s.file->hash);
            }
            outbound_.pop_front();
        }
    }
}
