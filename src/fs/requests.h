/*
 *   decode temp request
 *   TEMP = {
 *       user: string,
 *       id: option<bytes>,
 *       cid: option<bytes>,
 *       page: i32,
 *       perm_hash: bytes,
 *       expiration: u64,
 *       to: string
 *   }
 *
 *   if has cid
 *       if cid not public
 *           if perm hash exists(r.user, r.perm_hash)
 *               verify its a give signed by sender
 *               this means the requester has permission to recieve
 *
 *   send notification to user
 */


/*
 * -- SETTLE TEMP
 *
 *   decode temp response
 *   TEMP_RES = {
 *       accepted: bool,
 *       req_hash,
 *       length: i32,
 *       user: string,
 *   }
 *
 *   make sure there was a request matching req_hash sent
 *
 *   assert(req.to == sender)
 *
 *   if res.accepepted
 *       send response to user...
 *       however that is going to work...
 *   else
 *       also let them know but in a different way
 */
