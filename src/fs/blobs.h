
/*
 *   TODO -- I think there needs to be a get card handler
 *       like when someone requests a card...
 *       like from the past or whatever...
 *       But maybe we dont need that at this point...
 *
 *       What would that look like though??
 *           you would send proof of "GET" permissions
 *           then the person would queue a outbound file.
 *           or leave that with the person who it belongs to
 *           and they could fullfill it later if they want...
 *           and this would be like a full page fetch as well...
 *           because you arent just going to get a single older post...
 */

// fn handle_new_card() {
//
//     // unmarshal perm struct
//     let perm = Perm{
//         giver: [0u8;32],
//         recipient: [0u8;32],
//         get_id: [0u8;32],
//     };
//
//     // recalculate the permission hash
//     let mut hasher = blake3::Hasher::new();
//     hasher.update(&perm.giver);
//     hasher.update(&perm.recipient);
//     // hasher.update("GIVE");
//     let perm_hash = hasher.finalize();
//
//
//     // ensure perm_hash exists locally
//     if !social.db.exists(&perm_hash) {
//         // Return unauthorized error
//     }
//
//     // Generate random session ID
//     let id = random::random_b32();
//
//     // inform FS of:
//     //  - the incoming payload with ID == id
//     //  - giver key
//     //  - local key
//     //  - get_id
//     //
//     // it will then forward a get request to networker.
//     // letting the remote person know how to start streaming data.
// }
