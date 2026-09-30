# Node storage and sharing

FS is supposed to let people share content under explicit permissions.
A node can keep the bytes locally and arrange transfers with remote nodes.
Permissions and vouchers describe the relationships around those transfers.

FS manages stored blobs and the rules for sharing them.
Files are identified by their hashes.
The hash is also used to derive the storage path.
LMDB holds records used by the storage service.

## Transfers

A session tracks a file transfer.
It records the file, direction, byte count, and related voucher.
Open file handles can be shared by multiple sessions.
Outbound work is queued and sent through actor messages.

## Vouchers

A voucher refers to a file hash and size.
It identifies a sender and recipient and has an expiration time.
It also carries a signature.
The redemption paths use vouchers when arranging file transfers.
Incoming data is hashed as part of checking the transfer.

## Permissions

Permission records identify a giver and recipient.
The service has local and remote paths for giving permissions.
It also handles requests, settlement, and revocation.
Pending permissions are grouped by expiration time.

FS works with P2P to reach remote peers.
It sends notifications and errors back through actor messages.

## How sharing fits together

A permission records a grant between a giver and recipient.
A voucher identifies a particular file offered within that relationship.
The voucher handler checks that the permission and voucher agree.
It also checks their signatures and looks up the permission record.

Redeeming a voucher starts the work of obtaining the file.
The session gives later chunks an identity and tracks their progress.
Outbound transfers send chunks through messages rather than one giant buffer.
Incoming transfers accumulate bytes and hashing state until completion.

Local and remote handlers cover opposite sides of the exchange.
The node can act for its local users while communicating with a remote peer.
Shared open file handles avoid opening a separate descriptor for every session.

## References

- [node/fs/src/vouch.cpp:24](../../node/fs/src/vouch.cpp#L24) validates a voucher and its permission.
- [node/fs/src/vouch.cpp:118](../../node/fs/src/vouch.cpp#L118) starts the local redemption path.
- [node/fs/src/vouch.cpp:196](../../node/fs/src/vouch.cpp#L196) handles remote redemption.
- [node/fs/src/vouch.cpp:275](../../node/fs/src/vouch.cpp#L275) handles incoming file chunks.
- [node/fs/src/handlers.cpp:75](../../node/fs/src/handlers.cpp#L75) produces outbound transfer messages.
- [node/fs/src/perms.cpp:18](../../node/fs/src/perms.cpp#L18) starts the local permission grant path.
- [node/fs/src/perms.cpp:289](../../node/fs/src/perms.cpp#L289) handles local revocation.
- [node/fs/src/fs_types.h:41](../../node/fs/src/fs_types.h#L41) defines transfer session state.

## TODO

- Align FS requests with the database operations that store their records.
  [node/fs/src/vouch.cpp:111](../../node/fs/src/vouch.cpp#L111)
  [node/db/src/server.cpp:58](../../node/db/src/server.cpp#L58)

- Finish transfer handling so queued sessions advance through all chunks.
  [node/fs/src/handlers.cpp:75](../../node/fs/src/handlers.cpp#L75)

- The existing FS source notes discuss sequence numbers for permissions.
  [node/fs/note.txt:2](../../node/fs/note.txt#L2)
