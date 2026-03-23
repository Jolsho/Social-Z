

/*
 *   decode give request
 *   GIVE = {
 *       hash,
 *       to: string,
 *       to_cid: bytes,
 *       giver: String,
 *       giver_cid: bytes
 *       nonce: bytes
 *       signature: bytes
 *   }
 *
 *   give.to exists locally
 *
 *   give.giver == sender
 *   && is allowed to communicate
 *
 *   save the permission??
 *       SOMEHOW??
 *
 *   send notification to user if alive??
 */

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
