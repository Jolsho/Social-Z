#include "api/msgT.h"
#include "api/paths.h"
#include "api/types.h"
#include "crypto.h"
#include "manager.h"
#include "utils/vec.h"
#include <format>

HashT hash_perm(Perm* p) {
    Hasher h {};
    h.update(p->giver.b, KEY_SIZE);
    h.update(p->recipient.b, KEY_SIZE);
    h.update(p->nonce.b, NONCE_SIZE);
    h.update(p->data, PERM_DATA_SIZE);
    return h.finalize();
}

static constexpr uint8_t ACCEPTED{ 1 };

void FS::give_local(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) < sizeof(Perm)) {
        e.code = E_MALFORMED;
        e.msg  = "give_local() :: msg size too small";
        return;
    }

    Perm p; 
    vec_read(msg->data, p);

    if (!locals_.contains(p.giver)) {
        e.code = E_NOTLOCAL;
        e.msg = std::format("give_local() :: Not Local :: {}", key_to_str(p.recipient));
        return;
    }

    HashT p_hash = hash_perm(&p);
    if (!valid_signature(p.giver, p.signature, p_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "give_local() :: Fake Signature";
        return;
    }

    new_pending_perm(p_hash);

    Msg* m = consume_msg(free_out_msgs_);
    if (!m) {
        e.code = E_INTERNAL;
        e.msg = "give_local() :: too_main_ no msgs.";
        return;
    }

    m->priority = PRIORITY_WORK;
    m->too = ACTOR_P2P;
    m->code = P2P_BROADCAST;
    m->data = buffers_.grab(
        sizeof(PktCode) + sizeof(Actors) + 
        sizeof(uint64_t) + (KEY_SIZE * 1) + 
        sizeof(Perm)
    );
    vec_write(m->data, FS_GIVE);
    vec_write(m->data, ACTOR_FS);
    vec_write(m->data, static_cast<uint64_t>(1));
    vec_write(m->data, p.recipient.b);
    vec_write(m->data, p);
}

/// We receive a permission to post to a remote node.
void FS::give_remote(const Msg* msg, Error& e) {

    if (vec_remaining(msg->data) < sizeof(Perm)) {
        e.code = E_MALFORMED;
        e.msg  = "msg size small for give_remote()";
        return;
    }

    Perm p; 
    vec_read(msg->data, p);

    if (!locals_.contains(p.recipient)) {
        e.code = E_NOTLOCAL;
        e.msg = std::format("give_remote() :: Not Local :: {}", key_to_str(p.recipient));
        return;
    }

    HashT p_hash = hash_perm(&p);
    if (!valid_signature(p.giver, p.signature, p_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "give_remote() :: Fake Signature";
        return;
    }

    new_pending_perm(p_hash);

    Msg* m = consume_msg(free_out_msgs_);
    if (!m) {
        e.code = E_INTERNAL;
        e.msg = "give_remote() :: too_main_ no msgs.";
        return;
    }
    m->priority = PRIORITY_WORK;
    m->too = ACTOR_DB;
    m->data = buffers_.grab(sizeof(Perm) + sizeof(DB_PATH));
    vec_write(m->data, DB_PERM_INSERT);
    vec_write(m->data, p);
};

/// Remote accepts or denies a permission we offered them.
void FS::settle_remote(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) < sizeof(Perm) + 1 + SIGNATURE_SIZE) {
        e.code = E_MALFORMED;
        e.msg = "msg size small for settle_remote()";
        return;
    }

    Perm p{};
    vec_read(msg->data, p);
    
    HashT p_hash = hash_perm(&p);

    // ENSURE WE SENT AND CURRENTLY HOLD THE PERMISSION
    if (!locals_.contains(p.giver)) {
        e.code = E_NOTLOCAL;
        e.msg = std::format("settle_remote() :: Not Local :: {}", key_to_str(p.giver));
        return;
    }

    if (!valid_signature(p.giver, p.signature, p_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "settle_remote() :: Fake Signature";
        return;
    }

    // Extract whether the recipient has accepted the permission
    auto flags = vec_read<uint8_t>(msg->data);

    Hasher h;
    h.update(p_hash.b, HASH_SIZE);
    h.update(&flags, sizeof(uint8_t));
    HashT accept_hash = h.finalize();

    Signature sig;
    vec_read(msg->data, sig.b, SIGNATURE_SIZE);

    if (!valid_signature(p.recipient, sig, accept_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "settle_remote() :: Fake Signature";
        return;
    }


    int mcode = NOTI_DECLINED;
    if ((flags & ACCEPTED) == ACCEPTED) {
        mcode = NOTI_ACCEPTED;

        auto txn = db_.start_txn();
        e.r = db_.put(p_hash.b, HASH_SIZE, p_hash.b, HASH_SIZE, txn);
        db_.end_txn(txn, e.r);
        if (e.r != 0) {
            e.code = E_INTERNAL;
            e.msg = "settle_remote() :: Put perm hash";
            return;
        }
    }


    Msg* m = consume_msg(free_out_msgs_);
    if (!m) {
        e.code = E_INTERNAL;
        e.msg = "settle_remote() :: too_main_ no msgs.";
        return;
    }
    m->priority = PRIORITY_WORK;
    m->too = ACTOR_SZ;
    m->code = mcode;
    m->data = buffers_.grab(sizeof(Perm));
    vec_write(msg->data, p);
};

void FS::ask_local(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) < sizeof(Perm)) {
        e.code = E_MALFORMED;
        e.msg = "ask_local() :: msg size too small.";
        return;
    }

    Perm p {};
    vec_read(msg->data, p);
    HashT p_hash = hash_perm(&p);

    if (!locals_.contains(p.recipient)) {
        e.code = E_NOTLOCAL;
        e.msg = std::format("ask_local() :: Not Local :: {}", key_to_str(p.giver));
        return;
    }

    if (!valid_signature(p.recipient, p.signature, p_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "ask_local() :: Fake Signature";
        return;
    }

    Msg* m = consume_msg(free_out_msgs_);
    if (!m) {
        e.code = E_INTERNAL;
        e.msg = "ask_local() :: too_main_ no msgs.";
        return;
    }
    m->priority = PRIORITY_WORK;
    m->too = ACTOR_P2P;
    m->code = P2P_BROADCAST;
    m->data = buffers_.grab(
        sizeof(PktCode) + sizeof(Actors) + 
        sizeof(uint64_t) + (KEY_SIZE * 1) + 
        sizeof(Perm)
    );
    vec_write(m->data, FS_ASK);
    vec_write(m->data, ACTOR_FS);
    vec_write(m->data, static_cast<uint64_t>(1));
    vec_write(m->data, p.recipient.b);
    vec_write(m->data, p);
}


/// Remote asks for a specific permission.
void FS::ask_remote(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) < sizeof(Perm)) {
        e.code = E_MALFORMED;
        e.msg = "msg size small for ask_remote()";
        return;
    }

    Perm p {};
    vec_read(msg->data, p);

    HashT p_hash = hash_perm(&p);

    if (!locals_.contains(p.giver)) {
        e.code = E_NOTLOCAL;
        e.msg = std::format("ask_remote() :: Not Local :: {}", key_to_str(p.giver));
        return;
    }

    if (!valid_signature(p.recipient, p.signature, p_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "ask_remote() :: Fake Signature";
        return;
    }

    // Zero out the sig so local can sign it...
    // isn't mandatory, but whatever
    memset(&p.signature, 0, SIGNATURE_SIZE);


    Msg* m = consume_msg(free_out_msgs_);
    if (!m) {
        e.code = E_INTERNAL;
        e.msg = "ask_remote() :: too_main_ no msgs.";
        return;
    }

    m->priority = PRIORITY_WORK;
    m->too = ACTOR_DB;
    m->code = DB_PERM_INSERT;
    m->data = buffers_.grab(sizeof(Perm));

    vec_write(m->data, p);

    new_pending_perm(p_hash);
};


static constexpr unsigned char REVOKE{ 113 };

void FS::revoke_local(const Msg* msg, Error& e) {

    if (vec_remaining(msg->data) < sizeof(Perm) + SIGNATURE_SIZE) {
        e.code = E_MALFORMED;
        e.msg = "revoke_local() :: msg size too small.";
        return;
    }

    Perm p{};
    vec_read(msg->data, p);

    HashT p_hash = hash_perm(&p);

    if (!locals_.contains(p.recipient)) {
        e.code = E_NOTLOCAL;
        e.msg = std::format("revoke_local() :: Not Local :: {}", key_to_str(p.recipient));
        return;
    }


    Hasher h;
    h.update(p_hash.b, HASH_SIZE);
    h.update(&REVOKE, 1);
    HashT revoke_hash = h.finalize();

    Signature sig;
    vec_read(msg->data, sig.b, SIGNATURE_SIZE);

    if (!valid_signature(p.giver, sig, revoke_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "revoke_local() :: Fake Signature";
        return;
    }

    auto txn = db_.start_txn();
    e.r = db_.del(p_hash.b, HASH_SIZE, txn);
    db_.end_txn(txn, e.r);
    if (e.r != 0) {
        e.code = E_PERM_NOT_EXIST;
        e.msg = "revoke_local() :: Del perm :: Not Exist";
        return;
    }

    Msg* m = consume_msg(free_out_msgs_);
    if (!m) {
        e.code = E_INTERNAL;
        e.msg = "revoke_local() :: too_main_ no msgs.";
        return;
    }
    m->priority = PRIORITY_WORK;
    m->too = ACTOR_P2P;
    m->code = P2P_BROADCAST;
    m->data = buffers_.grab(
        sizeof(PktCode) + sizeof(Actors) + 
        sizeof(uint64_t) + (KEY_SIZE * 1) + 
        sizeof(Perm) + SIGNATURE_SIZE
    );
    vec_write(m->data, FS_REVOKE);
    vec_write(m->data, ACTOR_FS);
    vec_write(m->data, static_cast<uint64_t>(1));
    vec_write(m->data, p.recipient.b);
    vec_write(m->data, p);
    vec_write(m->data, sig);

}

/// Remote notifies us they revoked a permission we had.
void FS::revoke_remote(const Msg* msg, Error& e) {

    if (vec_remaining(msg->data) < sizeof(Perm) + SIGNATURE_SIZE) {
        e.code = E_MALFORMED;
        e.msg = "msg size small for revoke_remote()";
        return;
    }

    Perm p{};
    vec_read(msg->data, p);

    HashT p_hash = hash_perm(&p);

    if (!locals_.contains(p.recipient)) {
        e.code = E_NOTLOCAL;
        e.msg = std::format("{} :: revoke_remote() :: Not Local", key_to_str(p.recipient));
        return;
    }


    Hasher h;
    h.update(p_hash.b, HASH_SIZE);
    h.update(&REVOKE, 1);
    HashT revoke_hash = h.finalize();

    Signature sig;
    vec_read(msg->data, sig.b, SIGNATURE_SIZE);

    if (!valid_signature(p.giver, sig, revoke_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "revoke_remote() :: Fake Signature";
        return;
    }

    auto txn = db_.start_txn();
    e.r = db_.del(p_hash.b, HASH_SIZE, txn);
    db_.end_txn(txn, e.r);
    if (e.r != 0) {
        e.code = E_PERM_NOT_EXIST;
        e.msg = "revoke_remote() :: Del perm :: Not Exist";
        return;
    }
};



/// Local denies a give permission, meaning delete local copy.
/// In case of accept no action is needed. 
/// Perms are by default accepted.
void FS::settle_local(const Msg* msg, Error& e) {

    if (vec_remaining(msg->data) < sizeof(Perm) + 1 + SIGNATURE_SIZE) {
        e.code = E_MALFORMED;
        e.msg = "msg size small for settle_local()";
        return;
    }

    Perm p{};
    vec_read(msg->data, p);

    HashT p_hash = hash_perm(&p);

    // ENSURE WE SENT AND CURRENTLY HOLD THE PERMISSION
    if (!locals_.contains(p.recipient)) {
        e.code = E_NOTLOCAL;
        e.msg = std::format("settle_local() :: Not Local :: {}", key_to_str(p.giver));
        return;
    }

    // Extract whether the recipient has accepted the permission
    uint8_t flags;
    vec_read(msg->data, &flags, sizeof(uint8_t));

    Hasher h;
    h.update(p_hash.b, HASH_SIZE);
    h.update(&flags, sizeof(uint8_t));
    HashT accept_hash = h.finalize();

    Signature sig;
    vec_read(msg->data, sig.b, SIGNATURE_SIZE);

    if (!valid_signature(p.recipient, sig, accept_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "settle_local() :: Fake Signature";
        return;
    }


    int mcode = NOTI_DECLINED;
    if ((flags & ACCEPTED) == ACCEPTED) {
        auto txn = db_.start_txn();
        e.r = db_.put(p_hash.b, HASH_SIZE, p_hash.b, HASH_SIZE, txn);
        db_.end_txn(txn, e.r);
        if (e.r != 0) {
            e.code = E_INTERNAL;
            e.msg = "settle() :: Put perm hash";
            return;
        }
        mcode = NOTI_ACCEPTED;
    }

    Msg* m = consume_msg(free_out_msgs_);
    if (!m) {
        e.code = E_INTERNAL;
        e.msg = "settle_local() :: too_main_ no msgs.";
        return;
    }
    m->priority = PRIORITY_WORK;
    m->too = ACTOR_P2P;
    m->code = ACTOR_FS;
    m->data = buffers_.grab(sizeof(Perm) + sizeof(FS_PATH) + SIGNATURE_SIZE + 1);

    vec_write(m->data, FS_SETTLE);
    vec_write(m->data, p);
    vec_write(m->data, &flags);
    vec_write(m->data, sig);
};
