# Node database

DB is meant to make the node's structured information queryable.
It should answer questions about users and their published records.
Other services should request those operations without managing SQL themselves.

DB handles structured records for the node.
It opens SQLite and LMDB stores.
The service receives requests through its actor messages.

The SQLite schema defines users, ID cards, and notifications.
A user is identified by a public key.
Cards associate a user with a content hash and creation time.
Notifications also associate records with a user.

Prepared statements are reused for the implemented operations.
The current handlers insert users, delete users, and fetch recent cards.
Other operation codes exist but are not all handled here yet.

Responses use reusable buffers and outgoing actor messages.
Errors are intended to pass through the same service infrastructure.

DB is separate from FS.
DB handles structured records while FS manages file storage and sharing.

## Records and requests

User keys connect the records belonging to one person.
Cards refer to content by hash rather than storing file bytes in SQL.
Notifications are intended to track information waiting for a user.
Foreign keys associate these records with the user table.

Startup opens the stores and prepares the reusable SQL statements.
A handler translates a request into database work and response bytes.
The current dispatcher reads a length-prefixed textual path.
Other services also refer to the shared numeric DB operation codes.
Those interfaces still need to be brought into agreement.

## References

- [node/db/src/server.cpp:27](../../node/db/src/server.cpp#L27) opens the stores and initializes the service.
- [node/db/src/utils.hpp:20](../../node/db/src/utils.hpp#L20) contains the SQLite schema.
- [node/db/src/server.cpp:58](../../node/db/src/server.cpp#L58) registers the current request handlers.
- [node/db/src/server.cpp:71](../../node/db/src/server.cpp#L71) reads and dispatches a request path.
- [node/db/src/handlers.cpp:11](../../node/db/src/handlers.cpp#L11) inserts a user.
- [node/db/src/handlers.cpp:41](../../node/db/src/handlers.cpp#L41) retrieves recent cards.
- [common/include/sz_common/paths.h:14](../../common/include/sz_common/paths.h#L14) lists numeric DB operation codes.

## TODO

- Align textual request paths with the codes used by the other actors.
  [node/db/src/server.cpp:71](../../node/db/src/server.cpp#L71)

- Add the record operations required by permissions and blob storage.
  [common/include/sz_common/paths.h:20](../../common/include/sz_common/paths.h#L20)

- Finish error replies and schema initialization details.
  [node/db/src/server.cpp:96](../../node/db/src/server.cpp#L96)
  [node/db/src/utils.hpp:54](../../node/db/src/utils.hpp#L54)
