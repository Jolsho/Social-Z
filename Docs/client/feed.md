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

Fixed-size metadata entries follow directly after the header.
Each entry occupies 108 bytes.

| Post field | Bytes |
| --- | --- |
| Originator public key | 32 |
| Signed creation time | 8 |
| Encrypted package hash | 32 |
| Package decryption key | 32 |
| Blob count inside the package | 4 |

Blob count must be nonzero and uses the full u32 range.
The package hash identifies one opaque encrypted package.
Its internal content layout is a separate design step.
A recipient obtains its key from the voucher and retains it in their encrypted feed metadata.
The feed is protected by that user's data_key.
The package keeps its original package key and need not be rebuilt for private storage or delivery.

There are no empty slots or fixed post-count policy.
The plaintext page is limited to 64 KiB, including its 16-byte header.
That fits 606 complete entries, with a little unused room.
The development layout changed directly; its version remains 1 because the project is not deployed.

## Ownership and paging

FeedPage wraps the existing packed Feed with a page index.
Parsing copies the borrowed bytes and rebuilds the local offset/size index.
It converts stored timestamps and blob counts to their native representation used by PostView.
Replaced and destroyed packed data is wiped before freeing it, including the package keys.
Malformed input or allocation failure leaves the old page unchanged.
The destination must be zero-initialized or initialized with feed_page_init().
Initialize a fresh page and release it with feed_page_destroy().
Appending copies native packed posts into the page.
If a valid batch cannot fit, append returns FEED_PAGE_FULL without changing the page.
Publishing can then create a new page with the next index and append the new post there.
The codec itself does not advance indices or update the account header.

## References

- [client/src/codec/feed.h:15](../../client/src/codec/feed.h#L15) defines the page and ownership contract.
- [client/src/codec/feed.c:68](../../client/src/codec/feed.c#L68) enforces the page limit when appending.
- [client/src/codec/feed.c:87](../../client/src/codec/feed.c#L87) writes the plaintext page.
- [client/src/codec/feed.c:138](../../client/src/codec/feed.c#L138) validates and copies incoming plaintext.
- [client/tests/feed_page.c](../../client/tests/feed_page.c) checks the format and failure paths.

## TODO

- Define internal package framing and streaming import behavior.
- Connect authenticated encryption and verified blob retrieval.
- Publish page changes through signed owner updates.
  Page rollover must update the new page and account header atomically.
