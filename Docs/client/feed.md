# Feed pages

A feed page groups post references into one owner-managed record.
The client will encrypt it with user_data_key before storage.
The node only serves the resulting blob.
Encryption and retrieval are not connected yet.

## Layout

The plaintext begins with a 16-byte header.
All integers use big-endian byte order.

| Field | Bytes |
| --- | --- |
| Version, currently 1 | 2 |
| Record kind, feed page is 3 | 2 |
| Page index | 8 |
| Post count | 4 |

Variable-size posts follow directly after the header.
Each contains an originator key, a signed 64-bit creation time, a hash count, and the hashes.
Each hash identifies a data blob.
The first hash always points to the blob containing the remaining post data.
That blob currently holds mostly text and can hold richer post data later.
Private post-data blobs use user_data_key, so owner feed entries need no separate decryption key.
Publishing groups the plaintext contents of every referenced blob into one package.
The whole package is encrypted under one delivery key and waits for retrieval.
Its ciphertext has its own whole-blob hash and does not expose user_data_key.
Each recipient voucher contains the package delivery key.
Only its key subsection is encrypted using the sender's shared key with that recipient.
Signed node-readable fields identify the recipient, package, and permission.
The sender's signature binds those fields and the encrypted subsection.
Permissions and vouchers allow transfer with both users' consent and deferred retrieval.
One to five hashes make an entry 73 to 201 bytes long.
There are no empty slots or fixed post-count limit.
The whole plaintext page is limited to 64 KiB.

## Ownership and paging

FeedPage wraps the existing packed Feed with a page index.
Parsing copies the borrowed bytes and rebuilds the local offset/size index.
It converts stored timestamps to the native representation used by PostView.
Malformed input or allocation failure leaves the old page unchanged.
The destination must be zero-initialized or initialized with feed_page_init().
Initialize a fresh page and release it with feed_page_destroy().
Appending copies native packed posts into the page.
If a valid batch cannot fit, append returns FEED_PAGE_FULL without changing the page.
Publishing can then create a new page with the next index and append the new post there.
The codec itself does not advance indices or update the account header.

## References

- [client/src/codec/feed.h:15](../../client/src/codec/feed.h#L15) defines the page and ownership contract.
- [client/src/codec/feed.c:78](../../client/src/codec/feed.c#L78) enforces the page limit when appending.
- [client/src/codec/feed.c:95](../../client/src/codec/feed.c#L95) writes the plaintext page.
- [client/src/codec/feed.c:142](../../client/src/codec/feed.c#L142) validates and copies incoming plaintext.
- [client/tests/feed_page.c](../../client/tests/feed_page.c) checks the format and failure paths.

## TODO

- Define internal package framing and streaming import behavior.
- Connect authenticated encryption and verified blob retrieval.
- Publish page changes through signed owner updates.
  Page rollover must update the new page and account header atomically.
