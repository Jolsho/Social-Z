#include "fs/fs.h"
#include "fs/perms.h"
#include "msg.h"
#include "utils/keys.h"
#include <format>

/// We receive a permission to post to a remote node.
void fs::Manager::give(msg::Msg* msg) {
    if (msg->data.size() < PERM_SZ) {
        handle_err({
            .id = msg->id,
            .code = CODE::E_MALFORMED,
            .msg = "msg size small for give()"
        });
        return;
    }

    Perm p{ };
    std::byte* cursor = p.unmarshal(msg->data.data());

    if (!locals_.contains(p.recipient)) {
        handle_err({
            .id = msg->id,
            .code = CODE::E_NOTLOCAL,
            .msg = std::format("{} :: give() :: Not Local", key_to_str(p.recipient))
        });
        return;
    }

    Hash p_hash = p.hash();
    if (!valid_signature(p.giver, p.signature, p_hash)) {
        handle_err({
            .id = msg->id,
            .code = CODE::E_UNAUTHORIZED,
            .msg = "give() :: Fake Signature"
        });
        return;
    }

    auto txn = db_.start_txn();
    int r = db_.put(p_hash.data(), HASH_SIZE, &p, PERM_SZ, txn);
    db_.end_txn(txn, r);
    if (r != 0) {
        handle_err({
            .r = r,
            .id = msg->id,
            .code = CODE::E_INTERNAL,
            .msg = "give() :: Put perm hash"
        });
        return;
    }

    // TODO --> LET CLIENT KNOW SOMEHOW?
};


/// Remote accepts or denies a permission we offered them.
void fs::Manager::accept(msg::Msg* msg) {
    if (msg->data.size() < PERM_SZ + 1 + SIG_SIZE) {
        handle_err({
            .id = msg->id,
            .code = CODE::E_MALFORMED,
            .msg = "msg size small for accept()"
        });
        return;
    }

    Perm p{};
    std::byte* cursor = p.unmarshal(msg->data.data());
    Hash p_hash = p.hash();

    // ENSURE WE SENT AND CURRENTLY HOLD THE PERMISSION
    if (!locals_.contains(p.giver)) {
        handle_err({
            .id = msg->id,
            .code = CODE::E_NOTLOCAL,
            .msg = std::format("{} :: accept() :: Not Local", key_to_str(p.giver))
        });
        return;
    }

    if (!valid_signature(p.giver, p.signature, p_hash)) {
        handle_err({
            .id = msg->id,
            .code = CODE::E_UNAUTHORIZED,
            .msg = "accept() :: Fake Signature"
        });
        return;
    }

    auto txn = db_.start_rd_txn();
    int r = db_.exists(p_hash.data(), HASH_SIZE, txn);
    db_.end_txn(txn, r);
    if (r != 0) {
        handle_err({
            .r = r,
            .id = msg->id,
            .code = CODE::E_PERM_NOT_EXIST,
            .msg = "accept() :: Perm Not Exists"
        });
        return;
    }


    // Extract whether the recipient has accepted the permission
    std::byte flags = *cursor;
    cursor++;

    Hasher h;
    h.update(p_hash.data(), HASH_SIZE);
    h.update(&flags, 1);
    Hash accept_hash = h.finalize();

    Signature sig;
    memcpy(&sig, cursor, SIG_SIZE);

    if (!valid_signature(p.recipient, sig, accept_hash)) {
        handle_err({
            .id = msg->id,
            .code = CODE::E_UNAUTHORIZED,
            .msg = "accept() :: Fake Signature"
        });
        return;
    }

    static constexpr std::byte ACCEPTED{ 1 };

    if ((flags & ACCEPTED) == std::byte{0}) {
        auto txn = db_.start_txn();
        int r = db_.del(p_hash.data(), HASH_SIZE, txn);
        db_.end_txn(txn, r);
        if (r != 0) {
            handle_err({
                .r = r,
                .id = msg->id,
                .code = CODE::E_INTERNAL,
                .msg = "accept() :: Del perm hash"
            });
        }
    }

    /*
     * TODO
     *   send the notification 
     *      - so rpc can set permission as active or delete it
     */
};


/// Remote asks for a specific permission.
void fs::Manager::ask(msg::Msg* msg) {
    if (msg->data.size() < PERM_SZ) {
        handle_err({
            .id = msg->id,
            .code = CODE::E_MALFORMED,
            .msg = "msg size small for ask()"
        });
        return;
    }

    Perm p{};
    std::byte* cursor = p.unmarshal(msg->data.data());
    Hash p_hash = p.hash();

    if (!locals_.contains(p.giver)) {
        handle_err({
            .id = msg->id,
            .code = CODE::E_NOTLOCAL,
            .msg = std::format("{} :: ask() :: Not Local", key_to_str(p.giver))
        });
        return;
    }

    if (!valid_signature(p.recipient, p.signature, p_hash)) {
        handle_err({
            .id = msg->id,
            .code = CODE::E_UNAUTHORIZED,
            .msg = "ask() :: Fake Signature"
        });
        return;
    }

    // Zero out the sig so local can sign it...
    // isn't mandatory, but whatever
    memset(&p.signature, 0, SIG_SIZE);


    /*
     *  TODO
     *   Send the notification...
     *       find a way to store these?
     *       or do that in the go server
     */
};


/// Remote notifies us they revoked a permission we had.
void fs::Manager::revoke(msg::Msg* msg) {
    if (msg->data.size() < PERM_SZ + SIG_SIZE) {
        handle_err({
            .id = msg->id,
            .code = CODE::E_MALFORMED,
            .msg = "msg size small for revoke()"
        });
        return;
    }

    Perm p{};
    std::byte* cursor = p.unmarshal(msg->data.data());
    Hash p_hash = p.hash();

    if (!locals_.contains(p.recipient)) {
        handle_err({
            .id = msg->id,
            .code = CODE::E_NOTLOCAL,
            .msg = std::format("{} :: revoke() :: Not Local", key_to_str(p.recipient))
        });
        return;
    }

    static constexpr std::byte REVOKE{ 113 };

    Hasher h;
    h.update(p_hash.data(), HASH_SIZE);
    h.update(&REVOKE, 1);
    Hash revoke_hash = h.finalize();

    Signature sig;
    memcpy(&sig, cursor, SIG_SIZE);

    if (!valid_signature(p.giver, sig, revoke_hash)) {
        handle_err({
            .id = msg->id,
            .code = CODE::E_UNAUTHORIZED,
            .msg = "revoke() :: Fake Signature"
        });
        return;
    }

    auto txn = db_.start_txn();
    int r = db_.del(p_hash.data(), HASH_SIZE, txn);
    db_.end_txn(txn, r);
    if (r != 0) {
        handle_err({
            .r = r,
            .id = msg->id,
            .code = CODE::E_INTERNAL,
            .msg = "revoke() :: Del perm hash"
        });
    }

    // TODO -- notify RPC
};


