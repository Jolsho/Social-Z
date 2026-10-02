# Post packages

A package groups the post-data blob and its attachments before encryption.
The node serves the resulting ciphertext without interpreting these contents.
The package parser only reads decrypted plaintext.
It is not connected to retrieval, decryption, or client state yet.

## Format

All framing integers are big-endian.
The header contains version:u16, currently 1, and blob_count:u32.
The count must be nonzero.
Each blob then has a length:u64 followed by exactly that many bytes.
Empty blobs are allowed.
Blob zero contains the remaining post data, currently mostly text.
Its internal format and attachment references are still undecided.

## Parsing

Start with a zero-initialized PackageParser.
parse_package() reads up to one blob slice per call.
Advance the input by consumed and call again for any remaining bytes.
PACKAGE_BLOB supplies a PackageSlice with an index, total blob size, offset, and borrowed bytes.
An empty blob produces one slice with no bytes.
PACKAGE_MORE means the parser needs another chunk.
PACKAGE_DONE means the declared blobs have ended.
PACKAGE_ERR is terminal until the parser is reset.

Headers, lengths, and blob contents may cross input boundaries.
Only partial framing fields and progress are retained in the parser.
The parser allocates no storage and keeps no payload pointers between calls.
Copy slices before retaining them beyond the lifetime of their input buffer.

Pass all input to the parser, including any bytes after a returned slice.
Trailing bytes are rejected.
Call finish_package() at the actual end of input to reject incomplete packages.
Treat emitted slices as provisional until parsing and whole-package authentication succeed.
The parser itself performs no authentication or resource allocation.
Future consumers must apply their storage limits and check the feed metadata's blob count.

## Stored contents

Package owns an array of PackageBlob descriptors and one fixed-size plaintext buffer.
Each descriptor holds a native size and a borrowed pointer into that buffer.
Array position identifies the blob; position zero remains the post-data blob.
package_init() allocates zeroed descriptors and the shared buffer of the requested size.
Blob views are populated separately once their positions and lengths are known.
Neither individual blobs nor their byte pointers are freed separately.
package_destroy() wipes and frees the shared buffer, then releases the descriptors.
Eviction therefore releases a whole package, matching whole-package retrieval from the node.
All blob views become invalid when their package is destroyed.
This supplies ownership helpers, not a cache or parser-to-storage integration.
Future assembly must check u64 wire lengths against native allocation limits and buffer bounds.
The package hash and key remain in post metadata.

## References

- [client/src/content/package.h](../../client/src/content/package.h) defines owned package and blob descriptors.
- [client/src/content/package.c](../../client/src/content/package.c) initializes descriptors and releases plaintext storage.
- [client/tests/package_storage.c](../../client/tests/package_storage.c) checks shared views, allocation failures, and whole-buffer wiping.
- [client/src/codec/package.h](../../client/src/codec/package.h) defines parser state, slices, and the caller contract.
- [client/src/codec/package.c](../../client/src/codec/package.c) parses framing across arbitrary input chunks.
- [client/tests/package.c](../../client/tests/package.c) checks boundaries, truncation, trailing data, and large lengths.

## TODO

- Define the post-data blob and attachment reference format.
- Add package construction and the encrypted envelope.
- Connect authenticated input and client content storage.
