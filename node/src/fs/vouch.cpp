#include "fs/fs.h"
#include "fs/fs_types.h"
#include "msgT.h"
#include "utils/sig.h"
#include "utils/vec.h"
#include <cstddef>
#include <cstdio>
#include <format>
#include <sys/stat.h>
#include "db/handlers.h"


/// We are being given a voucher.
void fs::Manager::voucher(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) < PERM_SZ + VOUCHER_SZ) {
        e.code = E_MALFORMED;
        e.msg = "voucher() :: msg too small";
        return;
    }

    Perm p {};
    Voucher v{};
    if (
        !vec_read(msg->data, p) ||
        !vec_read(msg->data, v)
    ) {
        e.code = E_MALFORMED;
        e.msg = "voucher() :: malformed perm || voucher";
        return;
    }


    if (!locals_.contains(v.to)) {
        e.code = E_NOTLOCAL;
        e.msg = std::format("voucher() :: Not Local :: {}", key_to_str(p.recipient));
        return;
    }

    if (v.from != p.recipient || v.to != p.giver) {
        e.code = E_UNAUTHORIZED;
        e.msg = "voucher() :: voucher not match permission.";
        return;
    }

    HashT p_hash = p.hash();
    if (!valid_signature(v.to, p.signature, p_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "voucher() :: Fake Perm Signature";
        return;
    }

    auto txn = db_.start_rd_txn();
    int r = db_.exists(p_hash.b, HASH_SIZE, txn);
    db_.end_txn(txn, r);
    if (r == MDB_NOTFOUND) {
        e.code = E_PERM_NOT_EXIST;
        e.msg = "voucher() :: perm doesnt exist, or hasnt been accepted.";
        return;
    } else if (r != 0) {
        e.code = E_INTERNAL;
        e.msg = "voucher() :: getting perm failed.";
        return;
    }

    HashT v_hash = v.hash();
    if (!valid_signature(v.from, v.signature, v_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "voucher() :: Fake Voucher Signature";
        return;
    }

    
    Msg* m = chans_.out_.reserve(Priority::Work);
    if (!m) {
        e.code = E_INTERNAL;
        e.msg = "voucher() :: too_main_ no msgs.";
        return;
    }
    m->too = act_code(Actors::DB);
    m->data = buffers_.grab(VOUCHER_SZ + sizeof(DBPaths));

    if (
        !vec_write(m->data, DBPaths::VOUCHER_INSERT) ||
        !vec_write(m->data, p_hash) ||
        !vec_write(m->data, v)
    ) {
        e.code = E_INTERNAL;
        e.msg = "voucher() :: marshal response";
        buffers_.put(m->data);
        msg_wipe(m);
        return;
    }

    chans_.out_.commit(Priority::Work);
};


/// Someone is redeeming a voucher we sent them.
void fs::Manager::redeem(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) < VOUCHER_SZ + SID_SZ) {
        e.code = E_MALFORMED;
        e.msg = "redeem() :: msg too small";
        return;
    }

    Voucher v {};
    if (!vec_read(msg->data, v)) {
        e.code = E_MALFORMED;
        e.msg = "redeem() :: malformed  voucher";
        return;
    }

    if (!locals_.contains(v.from)) {
        e.code = E_NOTLOCAL;
        e.msg = std::format("redeem() :: Not Local :: {}", key_to_str(v.from));
        return;
    }


    if (v.expiration < time(nullptr)) {
        e.code = E_VOUCHER_EXPIRED;
        e.msg = std::format("redeem() :: VOUCHER_EXPIRED");
        return;
    }

    HashT v_hash = v.hash();
    if (!valid_signature(v.from, v.signature, v_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "redeem() :: Fake Voucher Signature";
        return;
    }

    Session session { 
        .is_inbound = false,
        .voucher = v,
        .actor = msg->from
    };
    if (!vec_read(msg->data, session.id.data(), SID_SZ)) {
        e.code = E_MALFORMED;
        e.msg = "redeem() :: msg too small :: sessionID";
        return;
    }

    FileHandle* fh;
    auto it = open_files_.find(v.file_hash);
    if (it == open_files_.end()) {
        std::string path = derive_path(v.file_hash);

        struct stat st;
        if (stat(path.data(), &st) != 0) {
            e.code = E_FILE_NOT_EXIST;
            e.msg = "redeem() :: File Not Exist, Stat";
            return;
        }
        
        FILE* f = fopen(path.data(), "wx");
        if (!f) {
            e.code = E_FILE_NOT_EXIST;
            e.msg = "redeem() :: File Not Exist, fopen";
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
    session.chunk_size = std::min(BufferSize::SU - SID_SZ - 1, fh->size);

    outbound_.push_back(session);
};


/// We are receiving a reward for a voucher we redeemed.
void fs::Manager::reward(const Msg* msg, Error& e) {
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
    s.hasher.update(msg->data->c, chunk_sz);

    // TODO -- need a way to account for missing chunks...
    // we can either automate a recovery by asking for those chunks...
    //  or we just scrap the entire thing and let the client initiate it again.

    size_t remaining = chunk_sz;
    unsigned char* c = msg->data->c;
    while (remaining > 0) {
        size_t n = fwrite(c, 1, remaining, s.file->f);
        if (n < 0) {
            e.code = E_INTERNAL;
            e.msg = "reward() :: Failed chunk write";

            fclose(s.file->f);
            sessions_.erase(id);
            open_files_.erase(s.file->hash);
            std::string path = derive_path(s.file->hash);
            remove(path.c_str());
            return;
        }
        remaining -= n;
        c += n;
    }

    s.chunk_idx++;

    // if last pkt erase session w
    if (s.chunk_idx * s.chunk_size >= s.file->size) {

        HashT& original_hash = s.file->hash;
        HashT final_hash = s.hasher.finalize();

        fclose(s.file->f);
        open_files_.erase(original_hash);
        sessions_.erase(id);

        if (final_hash != original_hash) {
            std::string path = derive_path(original_hash);
            remove(path.data());

            e.code = E_MALFORMED;
            e.msg = "reward() :: File hash doesnt match.";
            return;
        }

        Msg* m = chans_.out_.reserve(Priority::Work);
        m->too = act_code(Actors::DB);
        m->data = buffers_.grab(sizeof(DBPaths) + VOUCHER_SZ);
        vec_write(m->data, DBPaths::NEW_BLOB);
        vec_write(m->data, s.voucher);
        chans_.out_.commit(Priority::Work);
    }
};
