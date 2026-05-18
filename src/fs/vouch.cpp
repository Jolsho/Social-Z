#include "fs/fs.h"
#include "fs/perms.h"
#include "fs/vouch.h"
#include "utils/sig.h"
#include <cstddef>
#include <cstdio>
#include <format>
#include <sys/stat.h>

/// We are being given a voucher.
void fs::Manager::voucher(Msg* msg) {
    if (msg->data_len < PERM_SZ + VOUCHER_SZ) {
        handle_err({
            .id = msg->id,
            .code = Code::E_MALFORMED,
            .msg = "voucher() :: msg too small"
        });
        return;
    }

    Perm p {};
    std::byte* cursor = p.unmarshal(reinterpret_cast<std::byte*>(msg->data));

    Voucher v{};
    cursor = v.unmarshal(cursor);

    if (!locals_.contains(v.to)) {
        handle_err({
            .id = msg->id,
            .code = Code::E_NOTLOCAL,
            .msg = std::format("{} :: voucher() :: Not Local", key_to_str(p.recipient))
        });
        return;
    }

    if (v.from != p.recipient || v.to != p.giver) {
        handle_err({
            .id = msg->id,
            .code = Code::E_UNAUTHORIZED,
            .msg = "voucher() :: voucher not match permission."
        });
        return;
    }

    Hash p_hash = p.hash();
    if (!valid_signature(v.to, p.signature, p_hash)) {
        handle_err({
            .id = msg->id,
            .code = Code::E_UNAUTHORIZED,
            .msg = "voucher() :: Fake Perm Signature"
        });
        return;
    }

    auto txn = db_.start_rd_txn();
    bool exists = db_.exists(p_hash.data(), HASH_SIZE, txn);
    db_.end_txn(txn);
    if (!exists) {
        handle_err({
            .id = msg->id,
            .code = Code::E_PERM_NOT_EXIST,
            .msg = "voucher() :: perm doesnt exist"
        });
        return;
    }

    Hash v_hash = v.hash();
    if (!valid_signature(v.from, v.signature, v_hash)) {
        handle_err({
            .id = msg->id,
            .code = Code::E_UNAUTHORIZED,
            .msg = "voucher() :: Fake Voucher Signature"
        });
        return;
    }

    // TODO -- SEND TO RPC
};


/// Someone is redeeming a voucher we sent them.
void fs::Manager::redeem(Msg* msg) {
    if (msg->data_len < VOUCHER_SZ + SID_SZ) {
        handle_err({
            .id = msg->id,
            .code = Code::E_MALFORMED,
            .msg = "redeem() :: msg too small"
        });
        return;
    }

    Voucher v {};
    std::byte* cursor = v.unmarshal(reinterpret_cast<std::byte*>(msg->data));

    if (!locals_.contains(v.from)) {
        handle_err({
            .id = msg->id,
            .code = Code::E_NOTLOCAL,
            .msg = std::format("{} :: redeem() :: Not Local", key_to_str(v.from))
        });
        return;
    }


    if (v.expiration < time(nullptr)) {
        handle_err({
            .id = msg->id,
            .code = Code::E_VOUCHER_EXPIRED,
            .msg = std::format("redeem() :: VOUCHER_EXPIRED")
        });
        return;
    }

    Hash v_hash = v.hash();
    if (!valid_signature(v.from, v.signature, v_hash)) {
        handle_err({
            .id = msg->id,
            .code = Code::E_UNAUTHORIZED,
            .msg = "redeem() :: Fake Voucher Signature"
        });
        return;
    }

    Session session { .is_inbound = false };
    memcpy(&session.id, cursor, SID_SZ);
    cursor += SID_SZ;


    FileHandle* fh;
    auto it = open_files_.find(v.file_hash);
    if (it == open_files_.end()) {
        std::string path = derive_path(v.file_hash);

        struct stat st;
        if (stat(path.data(), &st) != 0) {
            handle_err({
                .id = msg->id,
                .code = Code::E_FILE_NOT_EXIST,
                .msg = "redeem() :: File Not Exist, Stat"
            });
            return;
        }
        
        FILE* f = fopen(path.data(), "wx");
        if (!f) {
            handle_err({
                .id = msg->id,
                .code = Code::E_FILE_NOT_EXIST,
                .msg = "redeem() :: File Not Exist, fopen"
            });
            return;
        }

        FileHandle ffh {
            .hash   = v.file_hash,
            .f      = f,
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
void fs::Manager::reward(Msg* msg) {
    if (msg->data_len <= SID_SZ) {
        handle_err({
            .id = msg->id,
            .code = Code::E_MALFORMED,
            .msg = "reward() :: msg too small"
        });
        return;
    }
    size_t chunk_sz = msg->data_len - SID_SZ;
    std::byte* cursor = reinterpret_cast<std::byte*>(msg->data);

    SessionID id;
    memcpy(&id, cursor, SID_SZ);
    cursor += SID_SZ;


    auto it = sessions_.find(id);
    if (it == sessions_.end()) {
        handle_err({
            .id = msg->id,
            .code = Code::E_UNAUTHORIZED,
            .msg = "reward() :: Session not exists"
        });
        return;
    }
    Session& s = it->second;

    s.hasher.update(cursor, chunk_sz - 1);

    size_t written = fwrite(cursor, 1, chunk_sz - 1, s.file->f);
    if (written != chunk_sz - 1) {
        handle_err({
            .id = msg->id,
            .code = Code::E_INTERNAL,
            .msg = "reward() :: Failed chunk write"
        });
        // TODO -- clean up the file??
        return;
    }

    s.chunk_idx++;

    // if last pkt erase session w
    if (s.chunk_idx * s.chunk_size >= s.file->size) {

        Hash final_hash = s.hasher.finalize();

        s.file->ref_count--;
        if (s.file->ref_count == 0) {
            fclose(s.file->f);
            if (final_hash != s.file->hash && !s.file->should_exist) {
                std::string path = derive_path(s.file->hash);
                remove(path.data());
            } else {
                s.file->should_exist = true;
            }
            open_files_.erase(s.file->hash);
        }

        sessions_.erase(id);
    }

    // TODO -- notify user??
};
