# Account header

The account header unlocks the user's identity and locates their private pages.
It contains the signing seed, data_key, feed-page indices, recipient-page indices, and inbox locator.
The password-derived key encrypts the whole header.
data_key encrypts the private pages and content blobs.
Login retrieves and decrypts this single header.

## Plaintext format

V1 starts with version:u16=1 and record_kind:u16=2.
Then come first_feed_page:u64 and current_feed_page:u64.
Follow with first_recipient_page:u64 and current_recipient_page:u64.
Next is inbox_head_locator:32, signing_seed:32, and data_key:32.
All integers use big-endian order.
The record is exactly 132 bytes.

Each first-page index must be at most its current-page index.
Zero and equal indices are accepted.
Page indices use the full u64 range.
The locator is copied as opaque bytes.
Checking that it resolves to a valid inbox remains separate work.

## Encryption and ownership

marshal_account_header() and parse_account_header() handle plaintext in caller-owned storage.
encrypt_account_header() creates the password-encrypted blob.
decrypt_account_header() authenticates it, validates page ranges, and recovers the signing identity.
The recovered public key must match the account key returned by username lookup.
Both the complete header and signing keys are installed only after success.
Failed calls leave outputs unchanged.
Borrowed inputs are never retained.
None of these functions allocate memory.
The password wrapper uses the general [buffer encryption](crypto.md) helper for its final record.

The encrypted blob is 213 bytes.
It uses the existing bounded Argon2id password profile and a single final secretstream record.
Every encryption generates a fresh salt and stream header.
Temporary plaintext, derived unlock keys, and stream state are wiped after use.
See [login](login.md) for the envelope and network ownership rules.

Plaintext marshaling, parsing, encryption, and decryption live together in client/src/codec/account.c.
Their declarations and AccountHeader type are private in client/src/codec/account.h.
The host uses client_login() rather than calling these codecs directly.
The node stores encrypted blobs and does not interpret account-header fields.
Node username resolution and actual transport routing remain unfinished.

## References

- [client/src/codec/account.h:16](../../client/src/codec/account.h#L16) defines the complete account header.
- [client/src/codec/account.c:74](../../client/src/codec/account.c#L74) marshals the plaintext record.
- [client/src/codec/account.c:109](../../client/src/codec/account.c#L109) parses and validates its fields.
- [client/src/codec/account.c:143](../../client/src/codec/account.c#L143) encrypts the header with the password-derived key.
- [client/src/codec/account.c:190](../../client/src/codec/account.c#L190) decrypts it and verifies the recovered identity.
- [client/tests/account.c:9](../../client/tests/account.c#L9) checks exact plaintext bytes and borrowed-input lifetime.
- [client/tests/login.c](../../client/tests/login.c) checks encrypted retrieval and failure handling.
