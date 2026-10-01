# Owner updates

An owner update replaces the blob referenced by an account or feed-page locator.
The locator stays stable while the blob hash and revision change.
Common defines the same marshaling, parsing, and signing rules for client and node.

## Record

V1 starts with version:u16=1 and record_kind:u16=1.
Then come owner_key:32 and identifier:u16.
Follow with index:u64, expected_revision:u64, and new_revision:u64.
Finish with blob_hash:32, blob_size:u64, and signature:64.
All integers use big-endian order.
The complete record is always 166 bytes.
OwnerIdentifier defines account=1, feed page=2, and recipient page=3.
Zero and unknown values are rejected.
The enum is encoded explicitly as u16, independently of its native C size.
Blob size must be nonzero.
The new revision must be exactly the expected revision plus one, without overflow.
Creation uses expected revision zero and new revision one.

## Signing and lookup

The signature covers BLAKE3("sz.record.v1" || encoded unsigned record).
The prefix excludes its terminating NUL.
The unsigned record includes every field before the signature, including version and kind.
sign_owner_update() checks that the signing key matches the named owner.
valid_owner_update() checks record structure and signature validity.
parse_owner_update() checks structure but does not authenticate the signature.
Failed calls leave their outputs unchanged.

The locator is BLAKE3("sz.locator.v1" || identifier:u16 || index:u64 || owner_key).
Blob details and revisions do not affect that locator.
The helper can derive a locator independently of a publication's revision or size.

## References

- [common/include/sz_common/owner.h:18](../../common/include/sz_common/owner.h#L18) defines the shared update and public helpers.
- [common/src/codec/owner.c:45](../../common/src/codec/owner.c#L45) marshals the record.
- [common/src/codec/owner.c:53](../../common/src/codec/owner.c#L53) parses bounded input.
- [common/tests/owner.c:21](../../common/tests/owner.c#L21) checks the wire format and hash preimages.

## TODO

Connect these helpers to client publication and FS handlers.
FS must verify the signature, local-user admission, and current stored revision.
It must also check blob existence, ownership, and quota before changing the locator.
A valid signature alone does not establish those storage conditions.
