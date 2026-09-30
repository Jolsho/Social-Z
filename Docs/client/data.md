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
It also contains hashes which refer to associated data.
The current format allows between one and five hashes.
The post itself does not contain all of that content.

## Blobs

Blobs hold the content referred to by hashes.
The client has a local store indexed by hash.
The blob parser looks for an existing item before assigning storage.
The store and buffer pool manage the memory behind these items.

## User data

There is a request and response path for user data.
The code includes password based key derivation and decryption.

The post request includes a user key and a feed offset.
The response parser appends posts to the feed.

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
- [client/src/data/post.c:13](../../client/src/data/post.c#L13) builds the request with a key and feed offset.
- [client/src/data/post.c:49](../../client/src/data/post.c#L49) parses a feed response.
- [client/src/data/blobs.c:9](../../client/src/data/blobs.c#L9) parses a blob into the local store.
- [client/src/utils/store.h:23](../../client/src/utils/store.h#L23) defines the store's memory and priority state.
- [client/src/utils/store.h:40](../../client/src/utils/store.h#L40) documents ownership when assigning an item.
- [client/src/data/user.c:46](../../client/src/data/user.c#L46) contains the user data decryption path.

## TODO

- Queue marshaled requests for sending.
  [client/src/data/post.c:44](../../client/src/data/post.c#L44)
  [client/src/data/user.c:41](../../client/src/data/user.c#L41)

- Finish parsing the decrypted user data.
  [client/src/data/user.c:105](../../client/src/data/user.c#L105)

- Use the response pagination flag to continue fetching feed pages.
  [client/src/data/post.c:57](../../client/src/data/post.c#L57)
