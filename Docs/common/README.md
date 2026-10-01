# Shared utilities

Common is meant to give the client and node a shared foundation.
The same key, hash, and message types should mean the same thing on both sides.
Containers and reusable memory support that work without belonging to one service.

Common contains the shared C types and utilities.
Both the node and client use it.
Public headers live under include/sz_common.

### structs

Vectors, hash tables, queues, priority queues, and buffer stores.
These provide the containers used by the other components.

### codec

Shared binary formats and serialization helpers.
Includes keys, hashes, cards, permissions, and vouchers.

### crypto

Key generation, signatures, and key encoding helpers.
Uses libsodium.
KeyPair holds a 64-byte Ed25519 signing secret and a 32-byte public key.
ExchangeKeyPair holds the separate 32-byte X25519 keys used for temporary P2P sessions.
Initial P2P exchange converts signing identities using libsodium's conversion functions.

### hash

Hashing helpers backed by BLAKE3.

paths.h defines operation codes used by the node services.
It gives messages a shared vocabulary for requesting work.

Common does not run a service loop of its own.
It supplies the building blocks used by the rest of the system.

## Why these types are shared

A hash identifies data across storage, network, and client code.
A key identifies the person or peer associated with that data.
Permissions and vouchers combine those identities with signed records.
Codec helpers provide the byte representations passed between components.

Hashing supports content identity and signature inputs.
Signature helpers let callers sign or check those hashes.
The buffer store lets services borrow and return reusable byte vectors.
The single producer single consumer queue supports communication boundaries.

## References

- [common/include/sz_common/codec.h:24](../../common/include/sz_common/codec.h#L24) begins the hash and key types.
- [common/include/sz_common/codec.h:65](../../common/include/sz_common/codec.h#L65) defines permission records.
- [common/include/sz_common/codec.h:78](../../common/include/sz_common/codec.h#L78) defines voucher records.
- [common/src/codec/fs.c:19](../../common/src/codec/fs.c#L19) hashes the signed permission fields.
- [common/src/crypto/crypto.c:26](../../common/src/crypto/crypto.c#L26) signs a hash.
- [common/src/hash/hash.c:17](../../common/src/hash/hash.c#L17) updates a streaming hash.
- [common/include/sz_common/buffers.h:50](../../common/include/sz_common/buffers.h#L50) returns a reusable buffer.
- [common/include/sz_common/queue.h:17](../../common/include/sz_common/queue.h#L17) defines the shared queue state.
