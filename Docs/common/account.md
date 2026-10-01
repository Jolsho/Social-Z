# Account header

The account header tells the client where to find its owner-managed pages.
Its plaintext contains feed-page indices, recipient-page indices, and the inbox-head locator.
It stays separate from the password-encrypted login record.
The main user_data_key recovered during login will encrypt this header.

## Format

V1 starts with version:u16=1 and record_kind:u16=2.
Then come first_feed_page:u64 and current_feed_page:u64.
Follow with first_recipient_page:u64 and current_recipient_page:u64.
Finish with inbox_head_locator:32.
All integers use big-endian order.
The record is exactly 68 bytes.

Each first-page index must be at most its current-page index.
Zero and equal indices are accepted.
Page indices use the full u64 range.
The locator is copied as opaque bytes.
Checking that it resolves to a valid inbox remains separate work.

## Ownership and validation

marshal_account_header() writes into caller-provided storage.
parse_account_header() copies fields from borrowed input.
Neither function allocates or retains the caller's memory.
Failed calls leave outputs unchanged.
Unsupported versions, wrong record kinds, truncated input, and trailing bytes are rejected.

This codec handles plaintext only.
Authenticated encryption and client header retrieval are not connected yet.

## References

- [common/include/sz_common/account.h:17](../../common/include/sz_common/account.h#L17) defines the shared header.
- [common/src/codec/account.c:31](../../common/src/codec/account.c#L31) marshals the record.
- [common/src/codec/account.c:62](../../common/src/codec/account.c#L62) parses and validates the record.
- [common/tests/account.c:9](../../common/tests/account.c#L9) checks the exact wire format and borrowed-input lifetime.
