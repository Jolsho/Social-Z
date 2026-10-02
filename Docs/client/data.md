# Client data

The data layer is supposed to hold useful content independently of its display.
A post can stay in a feed while its image or other content is fetched separately.
The same stored item can later be used by different views.

Data is kept separately from the entities used to display it.
A feed holds raw entries in a byte buffer.
An index records the offset and size of each entry.
This lets the client refer to entries without a separate allocation for each one.

## Posts

A post contains an originator key and a creation time.
It also contains hashes which refer to data blobs.
The first hash always refers to the blob containing the remaining post data.
That blob currently holds mostly text.
Richer post data can be added there later without embedding it in feed metadata.
The current format allows between one and five hashes.
An entry occupies 73 to 201 bytes.
The [feed-page codec](feed.md) packs these entries within a page byte limit.
The post itself does not contain all of that content.

## Blobs

Blobs hold the content referred to by hashes.
Private post-data blobs are encrypted using user_data_key.
Publishing groups all referenced plaintext blob contents into one delivery package.
It encrypts the whole package under one delivery key.
That package waits for authorized retrieval through permissions and vouchers.
The feed metadata does not need a separate key for each post.
The client has a local store indexed by hash.
The blob parser looks for an existing item before assigning storage.
The store and buffer pool manage the memory behind these items.

## Where the code lives

The data directory holds post views and packed feed storage.
codec/ holds the plaintext formats understood only by the client.
netwrk/marshalers/ builds requests and netwrk/parsers/ processes replies.
login.c owns the login workflow.

## User data

The user-data marshaler first requests a username lookup, then the encrypted login blob.
The user-data parser copies lookup fields and assembles incoming chunks through the blob parser.
Authenticated decryption recovers the stable signing seed and main user_data_key.
The password stays local.
See [login](login.md) for the bootstrap format and ownership rules.

The post request includes a user key and a feed offset.
The legacy response parser appends posts to the feed, but is currently disabled.
Encrypted feed-page retrieval is not wired yet.

## How content is represented

A post is a compact description of who published something and when.
Its hashes identify the associated content rather than embedding it.
The feed index lets layout code find an entry in the packed byte storage.
The blob store provides the bytes behind a content hash.

The store has a memory limit and priority information for eviction.
That is meant to let the client retain useful content within a fixed budget.
Its assignment interface takes ownership of the stored bytes.
Callers need to account for that when returning network buffers to the pool.

## References

- [client/src/data/post.h:18](../../client/src/data/post.h#L18) defines the post field offsets.
- [client/src/data/post.h:49](../../client/src/data/post.h#L49) checks the permitted hash count.
- [client/src/data/feed.h:19](../../client/src/data/feed.h#L19) defines packed feed storage and its index.
- [client/src/data/feed.c:9](../../client/src/data/feed.c#L9) implements entry appending.
- [client/src/netwrk/marshalers/post.c:13](../../client/src/netwrk/marshalers/post.c#L13) builds the request with a key and feed offset.
- [client/src/netwrk/parsers/post.c:11](../../client/src/netwrk/parsers/post.c#L11) parses a feed response.
- [client/src/netwrk/parsers/blob.c:11](../../client/src/netwrk/parsers/blob.c#L11) parses a blob into the local store.
- [client/src/utils/store.h:23](../../client/src/utils/store.h#L23) defines the store's memory and priority state.
- [client/src/utils/store.h:40](../../client/src/utils/store.h#L40) documents ownership when assigning an item.
- [client/src/netwrk/marshalers/user.c:10](../../client/src/netwrk/marshalers/user.c#L10) marshals username lookup and blob fetch requests.
- [client/src/netwrk/parsers/user.c:10](../../client/src/netwrk/parsers/user.c#L10) parses lookup replies and unlocks verified user data.

## TODO

- Queue marshaled requests for sending.
  [client/src/netwrk/marshalers/post.c:44](../../client/src/netwrk/marshalers/post.c#L44)

- Implement node username lookup and account-header retrieval.
  [login](login.md)

- Use the response pagination flag to continue fetching feed pages.
  [client/src/netwrk/parsers/post.c:20](../../client/src/netwrk/parsers/post.c#L20)
