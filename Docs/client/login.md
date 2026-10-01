# Login

Login retrieves encrypted user data rather than asking the node to check a password.
The username identifies the account.
Authenticated decryption unlocks its signing seed and main user_data_key.
The main key will protect the separate account header and owner-managed pages.

## Host interface

The host implements the existing send_request() hook.
Call client_login() with the username, password bytes, their length, and an output ContextID.
The client copies the password into its pool.
marshal_get_user_data_request() builds the lookup or fetch body in the context's send buffer.
The send hook receives that buffer with ownership of its pooled bytes.
Return the original buffer through client_return_buffer() after sending or copying those bytes.
The descriptor stays in the client's existing send-buffer array.
It must not be freed by the host.
Return all outstanding buffers before destroying the client.
A failed send can call client_cancel_login().

Pass replies to client_parse_response() with the supplied ContextID.
parse_user_data() validates lookup replies and delegates blob assembly to parse_blob().
Incoming response memory is borrowed only during parsing.
Retained lookup fields and chunks are copied into client-owned storage.
Replies may arrive synchronously or asynchronously.
If lookup arrives before its send buffer is returned, fetch waits for that return.
Cancellation and completion reserve a context until its host-held buffer is returned.

CLIENT_OK means more work is pending.
CLIENT_PARSE_DONE means login succeeded.
Failures close the login context without installing account keys.
client_is_logged_in() and client_get_public_key() expose the resulting public state.
client_cancel_login() wipes the pending password and releases partial retrieval state.
Shutdown also wipes pending passwords and retained account keys.
Use a fresh client to switch accounts.

## Bootstrap messages

Fixed integers below use big-endian encoding.
These are login request bodies for the host to route through its transport.
The generic node operation framing is still separate work.

Lookup request: version:u16=1, operation:u16=1, username_length:u16, username bytes.
Lookup reply: version:u16=1, operation:u16=1, account_key:32, blob_hash:32, blob_size:u32.
The reply is exactly 72 bytes and requires blob_size=149.
Fetch request: version:u16=1, operation:u16=2, blob_hash:32.
Fetch replies use the existing chunk format.
The first chunk carries the hash and native uint64_t total size before its payload.
Continuations carry the whole-blob hash and payload.
The client binds the expected hash and size before assembly and checks the completed ciphertext.

Usernames are passed through unchanged and limited to 64 bytes.
Passwords are between 1 and 1024 bytes.
The resolver remains responsible for username normalization, uniqueness, registration, and home-node routing.

## Encrypted login record

The 149-byte login blob has a password envelope followed by one final secretstream record.
Its public header is version:u16=1, suite:u16=1, Argon2id algorithm:u32, passes:u32, memory_bytes:u64.
Then come salt:16, stream_header:24, and ciphertext_length:u32=85.
V1 accepts only Argon2id 1.3 with two passes and 64 MiB.
Unsupported parameters are rejected before password derivation.
The header and length-prefixed username are authenticated additional data.

The 68-byte plaintext is version:u16=1, record_kind:u16=13, signing_seed:32, user_data_key:32.
The recovered signing identity must match the lookup's account key.
Plaintext, derived unlock keys, and stream state are wiped after decoding.
Only ciphertext enters the blob cache.

login_encrypt() creates a fresh salt and stream header.
Re-encrypting the same signing seed and data key under a new password preserves the account identity.
Publishing that replacement still requires the planned signed owner update.

## Remaining work

Implement the node-side username resolver and bounded pre-login access policy.
Connect the bootstrap bodies to the node-side resolver and operation framing.
Add protected local credential storage and password-change publication.
Registration, recovery, and client account-header retrieval are still unfinished.
The shared [account-header codec](../common/account.md) defines its plaintext format.
