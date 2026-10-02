# Buffer encryption

The crypto helper encrypts bytes without interpreting their contents.
An account header, feed page, or delivery package can use the same operations.
The caller chooses the key and marshals the plaintext first.
Password derivation remains in the account codec.

## In-place records

crypt_init_encrypt() starts a stream and writes its 24-byte header.
crypt_init_decrypt() starts the matching reader with that header and key.
encrypt() and decrypt() process one record inside a caller-owned Buffer.
They update its size and never allocate memory.
The buffer pointer and capacity stay unchanged.

A plaintext record is at most 64 KiB.
Encryption requires 17 additional bytes in the buffer.
Callers reserve that space before calling the helper.
Callers also store the stream header, frame record lengths, and provide authenticated additional data.
The account header retains its existing password envelope and username binding.
The planned stored-blob envelope remains separate framing work.

One context can process many records for a large blob.
Use final=false for intermediate records and final=true for the last one.
Transport chunks do not define these encrypted record boundaries.
A complete stream requires successful authentication of its final record.
Successful final records wipe and close the context.
crypt_clear() wipes an unfinished context when aborting an operation.

## Ownership and failure

Borrowed network or cached bytes must be copied into owned working storage before in-place decryption.
Additional data must stay outside the working buffer and context.
Invalid arguments or insufficient capacity leave the buffer unchanged.
Authentication failure or an unexpected record tag wipes the record and closes the context.
The buffer size becomes zero on those failures.
The caller must not install a page until decryption and plaintext parsing both succeed.

## References

- [client/src/utils/crypto.h](../../client/src/utils/crypto.h) defines the private context and buffer contract.
- [client/src/utils/crypto.c](../../client/src/utils/crypto.c) handles in-place records and state cleanup.
- [client/src/codec/account.c](../../client/src/codec/account.c) uses the helper after local password derivation.
- [client/tests/crypto.c](../../client/tests/crypto.c) checks capacities, exact overlap, tampering, record order, and feed codec reuse.
- [libsodium secretstream](https://doc.libsodium.org/secret-key_cryptography/secretstream) describes the underlying stream primitive.
