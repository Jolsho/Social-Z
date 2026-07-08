#include "sz/api/msgT.h"
#include "sz/api/paths.h"
#include "sz/codec.h"
#include "sz/crypto.h"
#include "manager.h"
#include "sz/hash.h"
#include "sz/utils/vec.h"


static constexpr uint8_t ACCEPTED{ 1 };

void FS::give_local(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) < sizeof(Perm)) {
        e.code = E_MALFORMED;
        e.msg  = "give_local() :: msg size too small";
        return;
    }

    Perm p{};
    unmarshal_perm(&msg->data->c, &p);

    if (!locals_.contains(p.giver)) {
        e.code = E_NOTLOCAL;
        e.msg = "give_local() :: Not Local";
        e.key = p.recipient;
        return;
    }

    HashT p_hash = hash_perm(&p);
    if (!valid_signature(&p.giver, &p.signature, &p_hash)) {
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
    m->data = grab_buff(buffers_, 
        sizeof(PktCode) + sizeof(Actors) + 
        sizeof(uint64_t) + (KEY_SIZE * 1) + 
        PERM_SIZE_NOPAD
    );

    FS_PATH p1 = FS_GIVE;
    Actors dst = ACTOR_FS;
    uint64_t recip_count = 1;

    vec_write(m->data, &p1, sizeof(FS_PATH));
    vec_write(m->data, &dst, sizeof(Actors));
    vec_write(m->data, &recip_count, sizeof(uint64_t));
    vec_write(m->data, p.recipient.b, KEY_SIZE);
    m->data->len += marshal_perm(&m->data->c, &p);
}

/// We receive a permission to post to a remote node.
void FS::give_remote(const Msg* msg, Error& e) {

    if (vec_remaining(msg->data) < sizeof(Perm)) {
        e.code = E_MALFORMED;
        e.msg  = "msg size small for give_remote()";
        return;
    }

    Perm p{};
    unmarshal_perm(&msg->data->c, &p);

    if (!locals_.contains(p.recipient)) {
        e.code = E_NOTLOCAL;
        e.msg = "give_remote() :: Not Local ";
        e.key = p.recipient;
        return;
    }

    HashT p_hash = hash_perm(&p);
    if (!valid_signature(&p.giver, &p.signature, &p_hash)) {
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
    m->data = grab_buff(buffers_, sizeof(DB_PATH) + PERM_SIZE_NOPAD);

    DB_PATH p1 = DB_PERM_INSERT;
    vec_write(m->data, &p1, sizeof(DB_PATH));

    m->data->len += marshal_perm(&m->data->c, &p);
};

/// Remote accepts or denies a permission we offered them.
void FS::settle_remote(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) < sizeof(Perm) + 1 + SIGNATURE_SIZE) {
        e.code = E_MALFORMED;
        e.msg = "msg size small for settle_remote()";
        return;
    }

    Perm p{};
    unmarshal_perm(&msg->data->c, &p);
    
    HashT p_hash = hash_perm(&p);

    // ENSURE WE SENT AND CURRENTLY HOLD THE PERMISSION
    if (!locals_.contains(p.giver)) {
        e.code = E_NOTLOCAL;
        e.msg = "settle_remote() :: Not Local ";
        e.key = p.giver;
        return;
    }

    if (!valid_signature(&p.giver, &p.signature, &p_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "settle_remote() :: Fake Signature";
        return;
    }

    // Extract whether the recipient has accepted the permission
    uint8_t flags;
    vec_read(msg->data, &flags, sizeof(uint8_t));

    Hasher h;
    hash_update(&h, p_hash.b, HASH_SIZE);
    hash_update(&h, &flags, sizeof(uint8_t));
    HashT accept_hash = hash_finalize(&h);

    Signature sig;
    vec_read(msg->data, sig.b, SIGNATURE_SIZE);

    if (!valid_signature(&p.recipient, &sig, &accept_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "settle_remote() :: Fake Signature";
        return;
    }


    int mcode = NOTI_DECLINED;
    if ((flags & ACCEPTED) == ACCEPTED) {
        mcode = NOTI_ACCEPTED;

        auto txn = start_txn(db_);
        e.r = put(db_, p_hash.b, HASH_SIZE, p_hash.b, HASH_SIZE, txn);
        end_txn(db_, txn, e.r);
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
    m->data = grab_buff(buffers_, PERM_SIZE_NOPAD);
    m->data->len += marshal_perm(&m->data->c, &p);
};

void FS::ask_local(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) < sizeof(Perm)) {
        e.code = E_MALFORMED;
        e.msg = "ask_local() :: msg size too small.";
        return;
    }

    Perm p{};
    unmarshal_perm(&msg->data->c, &p);
    HashT p_hash = hash_perm(&p);

    if (!locals_.contains(p.recipient)) {
        e.code = E_NOTLOCAL;
        e.msg = "ask_local() :: Not Local ";
        e.key = p.recipient;
        return;
    }

    if (!valid_signature(&p.recipient, &p.signature, &p_hash)) {
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
    m->data = grab_buff(buffers_, 
        sizeof(PktCode) + sizeof(Actors) + 
        sizeof(uint64_t) + (KEY_SIZE * 1) + 
        PERM_SIZE_NOPAD
    );
    FS_PATH p1 = FS_ASK;
    Actors dst = ACTOR_FS;
    uint64_t recip_count = 1;

    vec_write(m->data, &p1, sizeof(FS_PATH));
    vec_write(m->data, &dst, sizeof(Actors));
    vec_write(m->data, &recip_count, sizeof(uint64_t));
    vec_write(m->data, p.recipient.b, KEY_SIZE);
    m->data->len += marshal_perm(&m->data->c, &p);
}


/// Remote asks for a specific permission.
void FS::ask_remote(const Msg* msg, Error& e) {
    if (vec_remaining(msg->data) < sizeof(Perm)) {
        e.code = E_MALFORMED;
        e.msg = "msg size small for ask_remote()";
        return;
    }

    Perm p{};
    unmarshal_perm(&msg->data->c, &p);

    HashT p_hash = hash_perm(&p);

    if (!locals_.contains(p.giver)) {
        e.code = E_NOTLOCAL;
        e.msg = "ask_remote() :: Not Local ";
        e.key = p.giver;
        return;
    }

    if (!valid_signature(&p.recipient, &p.signature, &p_hash)) {
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
    m->data = grab_buff(buffers_, PERM_SIZE_NOPAD);
    m->data->len += marshal_perm(&m->data->c, &p);

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
    unmarshal_perm(&msg->data->c, &p);

    HashT p_hash = hash_perm(&p);

    if (!locals_.contains(p.recipient)) {
        e.code = E_NOTLOCAL;
        e.msg = "revoke_local() :: Not Local ";
        e.key = p.giver;
        return;
    }


    Hasher h;
    hash_update(&h, p_hash.b, HASH_SIZE);
    hash_update(&h, &REVOKE, 1);
    HashT revoke_hash = hash_finalize(&h);

    Signature sig;
    vec_read(msg->data, sig.b, SIGNATURE_SIZE);

    if (!valid_signature(&p.giver, &sig, &revoke_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "revoke_local() :: Fake Signature";
        return;
    }

    auto txn = start_txn(db_);
    e.r = del(db_, p_hash.b, HASH_SIZE, txn);
    end_txn(db_, txn, e.r);
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
    m->data = grab_buff(buffers_, 
        sizeof(PktCode) + sizeof(Actors) + 
        sizeof(uint64_t) + (KEY_SIZE * 1) + 
        PERM_SIZE_NOPAD + SIGNATURE_SIZE
    );
    FS_PATH p1 = FS_REVOKE;
    Actors dst = ACTOR_FS;
    uint64_t recip_count = 1;

    vec_write(m->data, &p1, sizeof(FS_PATH));
    vec_write(m->data, &dst, sizeof(Actors));
    vec_write(m->data, &recip_count, sizeof(uint64_t));
    vec_write(m->data, p.recipient.b, KEY_SIZE);

    m->data->len += marshal_perm(&m->data->c, &p);
    vec_write(m->data, sig.b, SIGNATURE_SIZE);

}

/// Remote notifies us they revoked a permission we had.
void FS::revoke_remote(const Msg* msg, Error& e) {

    if (vec_remaining(msg->data) < sizeof(Perm) + SIGNATURE_SIZE) {
        e.code = E_MALFORMED;
        e.msg = "msg size small for revoke_remote()";
        return;
    }

    Perm p{};
    unmarshal_perm(&msg->data->c, &p);

    HashT p_hash = hash_perm(&p);

    if (!locals_.contains(p.recipient)) {
        e.code = E_NOTLOCAL;
        e.msg = "revoke_remote() :: Not Local";
        e.key = p.giver;
        return;
    }


    Hasher h;
    hash_update(&h, p_hash.b, HASH_SIZE);
    hash_update(&h, &REVOKE, 1);
    HashT revoke_hash = hash_finalize(&h);

    Signature sig;
    vec_read(msg->data, sig.b, SIGNATURE_SIZE);

    if (!valid_signature(&p.giver, &sig, &revoke_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "revoke_remote() :: Fake Signature";
        return;
    }

    auto txn = start_txn(db_);
    e.r = del(db_, p_hash.b, HASH_SIZE, txn);
    end_txn(db_, txn, e.r);
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
    unmarshal_perm(&msg->data->c, &p);

    HashT p_hash = hash_perm(&p);

    // ENSURE WE SENT AND CURRENTLY HOLD THE PERMISSION
    if (!locals_.contains(p.recipient)) {
        e.code = E_NOTLOCAL;
        e.msg = "settle_local() :: Not Local";
        e.key = p.giver;
        return;
    }

    // Extract whether the recipient has accepted the permission
    uint8_t flags;
    vec_read(msg->data, &flags, sizeof(uint8_t));

    Hasher h = new_hasher();
    hash_update(&h, p_hash.b, HASH_SIZE);
    hash_update(&h, &flags, sizeof(uint8_t));
    HashT accept_hash = hash_finalize(&h);

    Signature sig;
    vec_read(msg->data, sig.b, SIGNATURE_SIZE);

    if (!valid_signature(&p.recipient, &sig, &accept_hash)) {
        e.code = E_UNAUTHORIZED;
        e.msg = "settle_local() :: Fake Signature";
        return;
    }


    int mcode = NOTI_DECLINED;
    if ((flags & ACCEPTED) == ACCEPTED) {
        auto txn = start_txn(db_);
        e.r = put(db_, p_hash.b, HASH_SIZE, p_hash.b, HASH_SIZE, txn);
        end_txn(db_, txn, e.r);
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
    m->data = grab_buff(buffers_, sizeof(FS_PATH) + PERM_SIZE_NOPAD + sizeof(flags) + SIGNATURE_SIZE);

    FS_PATH p1 = FS_SETTLE;
    vec_write(m->data, &p1, sizeof(FS_PATH));
    m->data->len += marshal_perm(&m->data->c, &p);
    vec_write(m->data, &flags, sizeof(uint8_t));
    vec_write(m->data, sig.b, SIGNATURE_SIZE);
};
