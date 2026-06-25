#include "api/paths.h"
#include "manager.h"
#include "fs_types.h"
#include "utils/vec.h"
#include <format>

static constexpr uint8_t ACCEPTED{ 1 };

void FS::local_give(const Msg* msg, Error& e) {
    // TODO
}

/// We receive a permission to post to a remote node.
void FS::give(const Msg* msg, Error& e) {

    if (vec_remaining(msg->data) < PERM_SZ) {
        e.code = E_MALFORMED;
        e.msg  = "msg size small for give()";
        return;
    }

    Perm p; 
    if (!vec_read(msg->data, p)) {
        e.code = E_MALFORMED;
        e.msg = "give() :: malformed perm";
        return;
    }

    if (!locals_.contains(p.recipient)) {
        e.code = E_NOTLOCAL;
        e.msg = std::format("give() :: Not Local :: {}", key_to_str(p.recipient));
        return;
    }

    HashT p_hash = p.hash();
    if (!valid_signature(p.giver, p.signature, p_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "give() :: Fake Signature";
        return;
    }


    new_pending_perm(p_hash);

    Msg* m = consume_msg(free_out_msgs_);
    if (!m) {
        e.code = E_INTERNAL;
        e.msg = "give() :: too_main_ no msgs.";
        return;
    }
    m->priority = PRIORITY_WORK;
    m->too = ACTOR_DB;
    m->data = buffers_.grab(PERM_SZ + sizeof(DB_PATH));
    if (
        !vec_write(m->data, DB_PERM_INSERT) ||
        !vec_write(m->data, p)
    ) {
        e.code = E_INTERNAL;
        e.msg = "give() :: marshal response";
        buffers_.put(m->data);
        msg_wipe(m);
        unconsume_msg(free_out_msgs_);
        return;
    }
};

/// Remote accepts or denies a permission we offered them.
void FS::settle(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) < PERM_SZ + 1 + SIG_SIZE) {
        e.code = E_MALFORMED;
        e.msg = "msg size small for remote_settle()";
        return;
    }

    Perm p{};
    if (!vec_read(msg->data, p)) {
        e.code = E_MALFORMED;
        e.msg = "remote_settle() :: malformed perm";
        return;
    }
    HashT p_hash = p.hash();

    // ENSURE WE SENT AND CURRENTLY HOLD THE PERMISSION
    if (!locals_.contains(p.giver)) {
        e.code = E_NOTLOCAL;
        e.msg = std::format("remote_settle() :: Not Local :: {}", key_to_str(p.giver));
        return;
    }

    if (!valid_signature(p.giver, p.signature, p_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "remote_settle() :: Fake Signature";
        return;
    }

    // Extract whether the recipient has accepted the permission
    auto flags = vec_read<uint8_t>(msg->data);

    Hasher h;
    h.update(p_hash.b, HASH_SIZE);
    h.update(&flags, sizeof(uint8_t));
    HashT accept_hash = h.finalize();

    Signature sig;
    vec_read(msg->data, sig.data(), SIG_SIZE);

    if (!valid_signature(p.recipient, sig, accept_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "remote_settle() :: Fake Signature";
        return;
    }


    int code = NOTI_DECLINED;
    if ((flags & ACCEPTED) == ACCEPTED) {
        code = NOTI_ACCEPTED;

        auto txn = db_.start_txn();
        e.r = db_.put(p_hash.b, HASH_SIZE, p_hash.b, HASH_SIZE, txn);
        db_.end_txn(txn, e.r);
        if (e.r != 0) {
            e.code = E_INTERNAL;
            e.msg = "remote_settle() :: Put perm hash";
            return;
        }
    }

    for (auto& pp: pending_perms_) {
        if (pp.remove_perm(p_hash)) break;
    }

    Msg* m = consume_msg(free_out_msgs_);
    if (!m) {
        e.code = E_INTERNAL;
        e.msg = "remote_settle() :: too_main_ no msgs.";
        return;
    }
    m->priority = PRIORITY_WORK;
    m->too = ACTOR_SZ;
    m->code = code;
    m->data = buffers_.grab(PERM_SZ);
    if (
        !vec_write(msg->data, p)
    ) {
        e.code = E_INTERNAL;
        e.msg = "remote_settle() :: marshal perm";
        buffers_.put(m->data);
        msg_wipe(m);
        unconsume_msg(free_out_msgs_);
        return;
    }
};

void FS::local_ask(const Msg* msg, Error& e) {
    // TODO
}

/// Remote asks for a specific permission.
void FS::ask(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) < PERM_SZ) {
        e.code = E_MALFORMED;
        e.msg = "msg size small for ask()";
        return;
    }

    Perm p {};
    if (!vec_read(msg->data, p)) {
        e.code = E_MALFORMED;
        e.msg = "ask() :: malformed perm";
        return;
    }
    HashT p_hash = p.hash();

    if (!locals_.contains(p.giver)) {
        e.code = E_NOTLOCAL;
        e.msg = std::format("ask() :: Not Local :: {}", key_to_str(p.giver));
        return;
    }

    if (!valid_signature(p.recipient, p.signature, p_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "ask() :: Fake Signature";
        return;
    }

    // Zero out the sig so local can sign it...
    // isn't mandatory, but whatever
    memset(&p.signature, 0, SIG_SIZE);


    Msg* m = consume_msg(free_out_msgs_);
    if (!m) {
        e.code = E_INTERNAL;
        e.msg = "ask() :: too_main_ no msgs.";
        return;
    }

    m->priority = PRIORITY_WORK;
    m->too = ACTOR_SZ;
    m->code = NOTI_PERM_REQ;
    m->data = buffers_.grab(PERM_SZ);
    if (
        !vec_write(m->data, p)
    ) {
        e.code = E_INTERNAL;
        e.msg = "ask() :: marshal perm";
        buffers_.put(m->data);
        msg_wipe(m);
        unconsume_msg(free_out_msgs_);
        return;
    }

    new_pending_perm(p_hash);
};


void FS::local_revoke(const Msg* msg, Error& e) {
    // TODO
}

/// Remote notifies us they revoked a permission we had.
void FS::revoke(const Msg* msg, Error& e) {

    // TODO -- 
    // How to recover when we don't get the notice of revokation?
    // like if this doesn't get called when it should then what??
    // I think we just suck it up...
    // maybe we make some function on the client where
    // you can say like inform person of permission state...
    // and like you can do that manually...
    // and this is in like a peer management system of sorts.

    if (vec_remaining(msg->data) < PERM_SZ + SIG_SIZE) {
        e.code = E_MALFORMED;
        e.msg = "msg size small for revoke()";
        return;
    }

    Perm p{};
    if (!vec_read(msg->data, p)) {
        e.code = E_MALFORMED;
        e.msg = "revoke() :: malformed perm";
        return;
    }
    HashT p_hash = p.hash();

    if (!locals_.contains(p.recipient)) {
        e.code = E_NOTLOCAL;
        e.msg = std::format("{} :: revoke() :: Not Local", key_to_str(p.recipient));
        return;
    }

    static constexpr unsigned char REVOKE{ 113 };

    Hasher h;
    h.update(p_hash.b, HASH_SIZE);
    h.update(&REVOKE, 1);
    HashT revoke_hash = h.finalize();

    Signature sig;
    vec_read(msg->data, sig.data(), SIG_SIZE);

    if (!valid_signature(p.giver, sig, revoke_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "revoke() :: Fake Signature";
        return;
    }

    auto txn = db_.start_txn();
    e.r = db_.del(p_hash.b, HASH_SIZE, txn);
    db_.end_txn(txn, e.r);
    if (e.r != 0) {
        e.code = E_PERM_NOT_EXIST;
        e.msg = "revoke() :: Del perm :: Not Exist";
        return;
    }
};



/// Local denies a give permission, meaning delete local copy.
/// In case of accept no action is needed. 
/// Perms are by default accepted.
void FS::local_settle(const Msg* msg, Error& e) {

    if (vec_remaining(msg->data) < PERM_SZ + 1 + SIG_SIZE) {
        e.code = E_MALFORMED;
        e.msg = "msg size small for local_settle()";
        return;
    }

    Perm p{};
    if (!vec_read(msg->data, p)) {
        e.code = E_MALFORMED;
        e.msg = "local_settle() :: malformed perm";
        return;
    }
    HashT p_hash = p.hash();

    // ENSURE WE SENT AND CURRENTLY HOLD THE PERMISSION
    if (!locals_.contains(p.recipient)) {
        e.code = E_NOTLOCAL;
        e.msg = std::format("local_settle() :: Not Local :: {}", key_to_str(p.giver));
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
    vec_read(msg->data, sig.data(), SIG_SIZE);

    if (!valid_signature(p.recipient, sig, accept_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "local_settle() :: Fake Signature";
        return;
    }


    int code = NOTI_DECLINED;
    if ((flags & ACCEPTED) == ACCEPTED) {
        auto txn = db_.start_txn();
        e.r = db_.put(p_hash.b, HASH_SIZE, p_hash.b, HASH_SIZE, txn);
        db_.end_txn(txn, e.r);
        if (e.r != 0) {
            e.code = E_INTERNAL;
            e.msg = "remote_settle() :: Put perm hash";
            return;
        }
    }

    for (auto& pp: pending_perms_) {
        if (pp.remove_perm(p_hash)) break;
    }

    Msg* m = consume_msg(free_out_msgs_);
    if (!m) {
        e.code = E_INTERNAL;
        e.msg = "local_settle() :: too_main_ no msgs.";
        return;
    }
    m->priority = PRIORITY_WORK;
    m->too = ACTOR_P2P;
    m->code = ACTOR_FS;
    m->data = buffers_.grab(PERM_SZ + sizeof(FS_PATH) + SIG_SIZE + 1);

    if (
        !vec_write(m->data, FS_SETTLE) ||
        !vec_write(m->data, p) ||
        !vec_write(m->data, &flags) ||
        !vec_write(m->data, sig)
    ) {
        e.code = E_INTERNAL;
        e.msg = "local_settle() :: marshal response";
        buffers_.put(m->data);
        msg_wipe(m);
        unconsume_msg(free_out_msgs_);
        return;
    }
};
