# Post packages

A package groups the post-data blob and its attachments before encryption.
The node serves the resulting ciphertext without interpreting these contents.
The package parser only reads decrypted plaintext.
Package retrieval connects blob transfer, decryption, and staged parsing.

## Memory limits

Encrypted packages are limited to 64 MiB, including encryption and package framing.
The client ciphertext cache has a 128 MiB budget, with index overhead deducted.
These settings live in client/include/sz_client/limits.h.
Plaintext packages occupy separate memory and remain owned by their callers.
Preparation retains complete ciphertext in caller-owned memory; persistence is deferred.
package_ciphertext_size() reports the allocation needed, or zero above the limit.
encrypt_package() fills that allocation using authenticated chunks and returns its complete hash.
Retrieval checks the ciphertext limit before allocation and keeps the caller's plaintext limit.

## Format

All framing integers are big-endian.
The header contains version:u16, currently 1, and blob_count:u32.
The count must be nonzero.
A table of blob_count lengths:u64 follows the header.
All blob payloads follow that table, in the same order.
Individual hashes are omitted; the whole encrypted package retains its hash.
Empty blobs are allowed.
Blob zero contains the remaining post data, currently mostly text.
Its internal format and attachment references are still undecided.

## Parsing

Start with an otherwise zero-initialized PackageParser whose max_size is the allowed complete plaintext size.
Pass a zero-initialized staging Package and keep both objects for the entire stream.
parse_package_contents() consumes each input chunk and copies payload slices into owned storage.
There is no separate borrowed-slice API.
Empty blobs remain descriptors with zero size.
PACKAGE_MORE means the parser needs another chunk.
PACKAGE_DONE means the declared blobs have ended.
PACKAGE_ERR is terminal until the parser is reset.

Headers, lengths, and blob contents may cross input boundaries.
Partial framing fields and progress stay in the parser.
The staging package holds the size table and owned payload buffer.
The parser allocates the descriptor array after validating the count against the byte limit.
It adds the sizes with bounds checks and allocates one payload buffer after the table is complete.
It retains no pointers into input buffers between calls.
The caller may reuse its input buffer after the call returns.

Pass each complete input chunk to the parser.
Trailing bytes are rejected.
Call finish_package() at the actual end of input to reject incomplete packages.
Treat assembled contents as provisional until parsing and whole-package authentication succeed.
The parser performs no authentication.
The caller supplies the plaintext byte limit and must check the feed metadata's expected blob count.

## Writing

Start with a zero-initialized PackageMarshaler.
marshal_package() fills a caller-owned output buffer and reports written bytes.
Call it again with the same marshaler and package until PACKAGE_DONE.
Headers, size fields, and payloads may split across output chunks.
Keep the package and its descriptors unchanged until writing finishes.
The output buffer must not overlap package storage or marshaler state.
The writer allocates no memory and performs no encryption.
It checks descriptor lengths before writing the first chunk.
Invalid input leaves output and progress unchanged.

## Stored contents

Package owns an array of PackageBlob descriptors and one fixed-size plaintext buffer.
Each descriptor holds a native size and a borrowed pointer into that buffer.
Array position identifies the blob; position zero remains the post-data blob.
package_init() allocates zeroed descriptors and the shared buffer of the requested size.
parse_package_contents() accepts successive input chunks and copies each parsed slice into the shared buffer.
Initialize the staging Package to zero; allocation follows the incoming count and size table.
The buffer holds only enclosed payload bytes, with each descriptor pointing at its own range.
Call finish_package() at end of input and destroy staging contents on failure.
Replace existing contents only after successful parsing and authentication.
Do not pass input that overlaps staging storage.
Neither individual blobs nor their byte pointers are freed separately.
package_destroy() wipes and frees the shared buffer, then releases the descriptors.
Eviction therefore releases a whole package, matching whole-package retrieval from the node.
All blob views become invalid when their package is destroyed.
This connects plaintext parsing to owned storage without adding decryption, retrieval, or cache policy.
Wire lengths are checked against the caller's byte limit before conversion to native allocation sizes.
The package hash and key remain in post metadata.

## References

- [client/src/content/package.h](../../client/src/content/package.h) defines owned package and blob descriptors.
- [client/src/content/package.c](../../client/src/content/package.c) initializes descriptors and releases plaintext storage.
- [client/tests/package_storage.c](../../client/tests/package_storage.c) checks shared views, allocation failures, and whole-buffer wiping.
- [client/src/codec/package.h](../../client/src/codec/package.h) defines parser state and the chunk-assembly contract.
- [client/src/codec/package.c](../../client/src/codec/package.c) parses framing across arbitrary input chunks.
- [client/tests/package.c](../../client/tests/package.c) checks boundaries, truncation, trailing data, and large lengths.

## TODO

- Define the post-data blob and attachment reference format.
- Add post-data construction and the encrypted envelope.
- Connect authenticated input and client content storage.
