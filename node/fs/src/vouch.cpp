/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/paths.h"
#include "sz_common/crypto.h"
#include "sz_common/codec.h"
#include "sz_node/fs.h"
#include "manager.h"
#include "fs_types.h"
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <sodium/randombytes.h>
#include <sys/stat.h>


/// We are being given a voucher.
void FS::voucher(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) < sizeof(Perm) + sizeof(Voucher)) {
        e.code = E_MALFORMED;
        e.msg = "voucher() :: msg too small";
        return;
    }

    Perm p {};
    unmarshal_perm(msg->data->c, &p);

    Voucher v{};
    unmarshal_voucher(msg->data->c, &v);


    if (!locals_.contains(v.to)) {
        e.code = E_NOTLOCAL;
        e.msg = "voucher() :: Not Local";
        e.key = p.recipient;
        return;
    }

    if (
        memcmp(v.from.b, p.recipient.b, KEY_SIZE) != 0 || 
        memcmp(v.to.b, p.giver.b, KEY_SIZE) != 0
    ) {
        e.code = E_UNAUTHORIZED;
        e.msg = "voucher() :: voucher not match permission.";
        return;
    }

    HashT p_hash = hash_perm(&p);
    if (!valid_signature(&v.to, &p.signature, &p_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "voucher() :: Fake Perm Signature";
        return;
    }

    auto txn = start_rd_txn(db_);
    int r = exists(db_, p_hash.b, HASH_SIZE, txn);
    end_txn(db_, txn, r);
    if (r == MDB_NOTFOUND) {
        e.code = E_PERM_NOT_EXIST;
        e.msg = "voucher() :: perm doesnt exist, or hasnt been accepted.";

        Msg* m = consume_msg(free_out_msgs_);
        if (!m) {
            e.code = E_INTERNAL;
            e.msg = "voucher() :: too_main_ no msgs.";
            return;
        }

        m->priority = PRIORITY_WORK;
        m->too = ACTOR_DB;
        m->data = grab_buff(buffers_, sizeof(Actors) + sizeof(FS_PATH) + sizeof(Perm));
        m->code = DB_NEW_TASK;

        Actors dst = ACTOR_FS;
        vec_write(m->data, &dst, sizeof(Actors));
        FS_PATH pth = FS_REVOKE;
        vec_write(m->data, &pth, sizeof(FS_PATH));
        m->data->len += marshal_perm(m->data->c, &p);

        return;
    } else if (r != 0) {
        e.code = E_INTERNAL;
        e.msg = "voucher() :: getting perm failed.";
        return;
    }

    HashT v_hash = hash_voucher(&v);
    if (!valid_signature(&v.from, &v.signature, &v_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "voucher() :: Fake Voucher Signature";
        return;
    }

    
    Msg* m = consume_msg(free_out_msgs_);
    if (!m) {
        e.code = E_INTERNAL;
        e.msg = "voucher() :: too_main_ no msgs.";
        return;
    }
    m->priority = PRIORITY_WORK;
    m->too = ACTOR_DB;
    m->data = grab_buff(buffers_, sizeof(DB_PATH) + HASH_SIZE + sizeof(Voucher));

    DB_PATH db_path = DB_VOUCHER_INSERT;
    vec_write(m->data, &db_path, sizeof(DB_PATH));
    vec_write(m->data, p_hash.b, HASH_SIZE);
    m->data->len += marshal_voucher(m->data->c, &v);
};


void FS::redeem_local(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) < sizeof(Voucher)) {
        e.code = E_MALFORMED;
        e.msg = "redeem_local() :: msg too small";
        return;
    }

    Voucher v{};
    unmarshal_voucher(msg->data->c, &v);

    if (!locals_.contains(v.to)) {
        e.code = E_NOTLOCAL;
        e.msg = "redeem_local() :: Not Local ";
        e.key = v.from;
        return;
    }

    if (allotted_space < total_fs_size + v.file_size) {
        e.code = E_OVERSIZED;
        e.msg = "redeem_local() :: allotted fs space would be exceeded by new file";
        return;
    }

    SessionID id;

    randombytes_buf(id.data(), id.size());
    Session& s = sessions_[id];;
    while (s.id == id) {
        randombytes_buf(id.data(), id.size());
        s = sessions_[id];
    }

    s.id = id;
    s.actor = msg->from;
    s.voucher = v;
    s.byte_count = 0;
    s.is_inbound = true;

    std::string path = tmp_path(id);
    
    int fd = open(path.data(), O_RDWR | O_CREAT | O_EXCL, 0664);
    if (fd < 0) {
        if (errno == EEXIST) {
            Msg* m = consume_msg(free_out_msgs_);
            if (!m) {
                e.code = E_INTERNAL;
                e.msg = "reward() :: too_main_ no msgs.";
                return;
            }
            m->priority = PRIORITY_WORK;
            m->too = ACTOR_DB;
            m->data = grab_buff(buffers_, sizeof(DB_PATH) + sizeof(Voucher));
            DB_PATH db_path = DB_NEW_BLOB;
            vec_write(m->data, &db_path, sizeof(DB_PATH));
            m->data->len += marshal_voucher(m->data->c, &v);

            sessions_.erase(id);
            return;
        }

        e.code = E_INTERNAL;
        e.msg = "redeem_local() :: failed to create file.";
        return;
    }

    FileHandle ffh {
        .hash   = v.file_hash,
        .fd     = fd,
        .size   = v.file_size,
        .ref_count = 1,
    };

    s.file = &ffh;
    open_files_.insert({v.file_hash, ffh});

}

/// Someone is redeeming a voucher we sent them.
void FS::redeem_remote(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) < sizeof(Voucher) + SID_SZ) {
        e.code = E_MALFORMED;
        e.msg = "redeem_remote() :: msg too small";
        return;
    }

    Voucher v{};
    unmarshal_voucher(msg->data->c, &v);

    if (!locals_.contains(v.from)) {
        e.code = E_NOTLOCAL;
        e.msg = "redeem_remote() :: Not Local ";
        e.key = v.from;
        return;
    }

    if (v.expiration < time(nullptr)) {
        e.code = E_VOUCHER_EXPIRED;
        e.msg = std::format("redeem_remote() :: VOUCHER_EXPIRED").c_str();
        return;
    }

    HashT v_hash = hash_voucher(&v);
    if (!valid_signature(&v.from, &v.signature, &v_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "redeem_remote() :: Fake Voucher Signature";
        return;
    }

    Session session { 
        .is_inbound = false,
        .voucher = v,
        .actor = msg->from
    };

    vec_read(msg->data, session.id.data(), SID_SZ);

    FileHandle* fh;
    auto it = open_files_.find(v.file_hash);
    if (it == open_files_.end()) {
        std::string path = derive_path(v.file_hash);

        struct stat st;
        if (stat(path.data(), &st) != 0) {
            e.code = E_FILE_NOT_EXIST;
            e.msg = "redeem_remote() :: File Not Exist, Stat";
            return;
        }
        
        int fd = open(path.data(), O_RDONLY);
        if (fd < 0) {
            e.code = E_FILE_NOT_EXIST;
            e.msg = "redeem_remote() :: File Not Exist, fopen";
            return;
        }

        FileHandle ffh {
            .hash   = v.file_hash,
            .fd      = fd,
            .size   = static_cast<size_t>(st.st_size),
            .ref_count = 1,
        };

        fh = &ffh;
        open_files_.insert({v.file_hash, ffh});

    } else {
        fh = &it->second;
        fh->ref_count++;
    }

    session.file = fh;

    outbound_.push_back(session);
};


/// We are receiving a reward for a voucher we redeemed.
void FS::reward(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) <= sizeof(uint64_t) + SID_SZ) {
        e.code = E_MALFORMED;
        e.msg = "reward() :: msg too small";
        return;
    }

    SessionID id;
    vec_read(msg->data, id.data(), SID_SZ);

    auto it = sessions_.find(id);
    if (it == sessions_.end()) {
        e.code = E_UNAUTHORIZED;
        e.msg = "reward() :: Session not exists";
        return;
    }
    Session& s = it->second;

    uint64_t chunk_sz = vec_remaining(msg->data);
    hash_update(&s.hasher, msg->data->c, chunk_sz);

    size_t remaining = chunk_sz;
    uint8_t* c = msg->data->c;
    while (remaining > 0) {
        size_t n = write(s.file->fd, c, remaining);
        if (n < 0) {
            e.code = E_INTERNAL;
            e.msg = "reward() :: Failed chunk write";

            close(s.file->fd);
            open_files_.erase(s.file->hash);
            remove(tmp_path(s.id).data());
            sessions_.erase(id);
            return;
        }
        remaining -= n;
        c += n;
        s.byte_count += n;
    }


    // if last pkt erase session w
    if (s.byte_count >= s.file->size) {

        HashT& original_hash = s.file->hash;
        HashT final_hash = hash_finalize(&s.hasher);

        close(s.file->fd);
        open_files_.erase(original_hash);

        std::string tmp = tmp_path(s.id);
        if (final_hash != original_hash) {
            e.code = E_MALFORMED;
            e.msg = "reward() :: File hash doesnt match.";

            remove(tmp.data());
            sessions_.erase(id);
            return;
        }

        std::string& real = derive_path(s.file->hash);
        if (rename(tmp.data(), real.data()) < 0) {
            e.code = E_INTERNAL;
            e.msg = "reward() :: Failed to rename file.";

            remove(tmp.data());
            sessions_.erase(id);
            return;
        }

        Msg* m = consume_msg(free_out_msgs_);
        if (!m) {
            e.code = E_INTERNAL;
            e.msg = "reward() :: too_main_ no msgs.";

            remove(real.data());
            sessions_.erase(id);
            return;
        }

        m->priority = PRIORITY_WORK;
        m->too = ACTOR_DB;
        m->data = grab_buff(buffers_, sizeof(DB_PATH) + sizeof(Voucher));
        DB_PATH db_path = DB_NEW_BLOB;
        vec_write(m->data, &db_path, sizeof(DB_PATH));
        m->data->len += marshal_voucher(m->data->c, &s.voucher);

        total_fs_size += s.byte_count;

        sessions_.erase(id);
    }
};
