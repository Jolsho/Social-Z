# Login

Login retrieves the encrypted account header rather than asking the node to check a password.
The username identifies the account.
Authenticated decryption unlocks its signing seed, data_key, and private page references.
The password-derived key protects this header.
data_key protects owner-managed pages and private account data.
Post packages have their own keys retained in encrypted feed metadata.

## Host interface

The host implements the existing send_request() hook.
Call client_login() with the username, password bytes, their length, and an output ContextID.
The client copies the password into its pool.
login_send_request() in operations/login.c uses marshal_account_request() and sends one username request.
The request uses the context's send buffer.
login.c coordinates sending, credentials, and cancellation.
The send hook receives that buffer with ownership of its pooled bytes.
Return the original buffer through client_return_buffer() after sending or copying those bytes.
The descriptor stays in the client's existing send-buffer array.
It must not be freed by the host.
Return all outstanding buffers before destroying the client.
A failed send can call client_cancel_login().

Pass replies to client_parse_response() with the supplied ContextID.
login_handle_response() in operations/login.c parses response metadata through parse_account_response_metadata().
It delegates blob assembly to parse_blob() in codec/blob.c.
Incoming response memory is borrowed only during parsing.
Retained metadata fields and chunks are copied into client-owned storage.
Replies may arrive synchronously or asynchronously.
The node resolves the username and sends metadata followed by encrypted header chunks.
No second request is sent, and replies can finish before the outgoing buffer is returned.
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

Account request: version:u16=1, operation:u16=1, username_length:u16, username bytes.
Response metadata: version:u16=1, operation:u16=1, account_key:32, blob_hash:32, blob_size:u32.
The reply is exactly 72 bytes and requires blob_size=213.
After the response metadata, the node sends the encrypted header using the existing chunk format.
The first chunk carries the hash and native uint64_t total size before its payload.
Continuations carry the whole-blob hash and payload.
The client binds the expected hash and size before assembly and checks the completed ciphertext.

Usernames are passed through unchanged and limited to 64 bytes.
Passwords are between 1 and 1024 bytes.
The resolver remains responsible for username normalization, uniqueness, registration, and home-node routing.

## Encrypted account header

The 213-byte account blob has a password envelope followed by one final secretstream record.
Its public header is version:u16=1, suite:u16=1, Argon2id algorithm:u32, passes:u32, memory_bytes:u64.
Then come salt:16, stream_header:24, and ciphertext_length:u32=149.
V1 accepts only Argon2id 1.3 with two passes and 64 MiB.
Unsupported parameters are rejected before password derivation.
The header and length-prefixed username are authenticated additional data.

The 132-byte plaintext uses version:u16=1 and record_kind:u16=2.
It contains page indices, the inbox locator, signing_seed, and data_key.
See [account header](account.md) for the exact field order.
The recovered signing identity must match the response's account key.
Plaintext, derived unlock keys, and stream state are wiped after decoding.
Only ciphertext enters the blob cache.

encrypt_account_header() creates a fresh salt and stream header.
Re-encrypting the same signing seed and data key under a new password preserves the account identity.
Publishing that replacement still requires the planned signed owner update.

## Remaining work

Implement the node-side username resolver and bounded pre-login access policy.
Connect the bootstrap bodies to the node-side resolver and operation framing.
Add protected local credential storage and password-change publication.
Registration, recovery, and retrieval of the referenced private pages are still unfinished.
Password encryption and plaintext interpretation live together in client/src/codec/account.c.
client/src/operations/login.c coordinates requests, responses, and cancellation.
Private username request and response metadata helpers live in client/src/codec/login.c and login.h.
Common contains no login API.
The node will decode its small request bodies in the relevant handler.
The stream encryption follows the [libsodium secretstream API](https://doc.libsodium.org/secret-key_cryptography/secretstream).
