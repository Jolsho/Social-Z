#include "fs/fs.h"
#include "fs/perms.h"
#include "msg.h"
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

    Perm p{};
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

    // TODO -- need to maintain a list of perms like this for an individual...
    // people need to know what perms they have...
    // or maybe we do that in RPC??
    // what if RPC holds ordered data?
    // like card hashes and what not??
    // that could be interesting??
    //
    // I think we just store hashes on this side.
    // then the RPC can store full perms/vouchers etc.

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


/// Remote accepts a permission we offered them.
void fs::Manager::accept(msg::Msg* msg) {
/*
 *   decode settlegive
 *   settle_give = {
 *       hash,
 *       accepted: bool
 *   }
 *
 *   if not settle_give.accepted
 *       delete the pending give??
 *       return
 *
 *   set permission as active
 *
 *   send the notification
 */
};


/// Remote asks for a specific permission.
void fs::Manager::ask(msg::Msg* msg) {
/*
 *   decode ask
 *   ask = {
 *       asker: string,
 *       asked: string,
 *       asker_cid: bytes,
 *       local_cid: bytes
 *   }
 *
 *   validate ask.asker == sender
 *
 *   validate ask.asked exists locally
 *
 *   Send the notification...
 *       find a way to store these?
 *       or do that in the go server
 *
 */
};


/// Remote notifies us they revoked a permission we had.
void fs::Manager::revoke(msg::Msg* msg) {
/*
 *   decode revoke
 *   revoke = {
 *       hash,
 *       signature,
 *   }
 *
 *   verify revoke_signature = H("revoke" + revoke.hash)
 *   came from sender
 *
 *   retrieve the permission being revoked local signature
 *   if it matches with sender we delete it
 *
 *   somehow get the person who had the permissions address...
 *   then we can notify them...
 */
};


