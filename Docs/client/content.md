# Client content

The content layer is supposed to hold useful content independently of its display.
A post can stay in a feed while its image or other content is fetched separately.
The same stored item can later be used by different views.

Data is kept separately from the entities used to display it.
A feed page holds fixed-size entries in one packed byte buffer.
An entry is found directly at its position multiplied by POST_SIZE.
This lets the client refer to entries without a separate allocation for each one.

## Posts

A post entry is metadata for one encrypted package.
It holds the originator, creation time, package hash, package key, and blob count.
Each entry occupies 108 bytes.
The [feed-page codec](feed.md) packs these entries within a page byte limit.
The feed's encryption under the user's data_key protects the retained package keys.

## Packages and blobs

The package contains the actual post data and its enclosed blobs.
Its internal format and creation are still separate work.
The node serves the encrypted package by its whole-blob hash without interpreting it.
A recipient recovers the package key from a voucher's encrypted subsection.
They retain that key with the post metadata rather than rebuilding the package under data_key.
Both author and recipients can keep the same encrypted package.

The client has a local ciphertext store indexed by hash.
The blob parser looks for an existing item before assigning storage.
The store and buffer pool manage the memory behind these items.
Packed feed data is wiped before replacement or destruction because it contains package keys.

## Where the code lives

The content directory holds PostView and FeedPage.
FeedPage owns packed post storage, its size and capacity, and a page_number.
Feed owns a vector of loaded pages and tracks the current page number.
Its navigation functions are declared but not implemented yet.
codec/ holds the plaintext formats understood only by the client.
netwrk/marshalers/ builds requests and netwrk/parsers/ processes replies.
login.c owns the login workflow.

## User data

The user-data marshaler first requests a username lookup, then the encrypted login blob.
The user-data parser copies lookup fields and assembles incoming chunks through the blob parser.
Authenticated decryption recovers one account header with the stable signing seed, data_key, and page references.
The password stays local.
See [login](login.md) for the bootstrap format and ownership rules.

The post request includes a user key and a feed offset.
The legacy response parser is disabled and rejects calls until feed-page retrieval is connected.
Encrypted feed-page retrieval is not wired yet.

## How content is represented

A post is a compact description of who published something and when.
Its package hash identifies the encrypted content rather than embedding it.
Fixed-size addressing lets layout code find an entry without an offset index.
The blob store provides the bytes behind a content hash.

The store has a memory limit and priority information for eviction.
That is meant to let the client retain useful content within a fixed budget.
Its assignment interface takes ownership of the stored bytes.
Callers need to account for that when returning network buffers to the pool.

## References

- [client/src/content/post.h:17](../../client/src/content/post.h#L17) defines the post field offsets.
- [client/src/content/post.h:55](../../client/src/content/post.h#L55) rejects zero blob counts.
- [client/src/content/feed_page.h](../../client/src/content/feed_page.h) defines the page and direct post access.
- [client/src/content/feed_page.c](../../client/src/content/feed_page.c) appends entries and owns their storage.
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
