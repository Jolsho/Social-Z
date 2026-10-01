# Peer to peer networking

P2P is meant to let community members communicate directly with one another.
It should hide socket and packet handling from the other node services.
Those services should be able to request remote work through actor messages.

P2P handles communication between nodes.
It owns the TCP listener and the active peer connections.
Socket events are processed through the actor event loop.

## Connections

Connections are indexed by socket, connection ID, and peer key.
Each connection tracks its protocol state and pending packets.
Timeouts limit negotiation and idle connection lifetimes.

## Handshake

The handshake checks the remote key against the citizen records.
It also checks whether that citizen is considered trustworthy.
Keys are exchanged to establish receive and transmit session keys.
Peer identities use Ed25519 signing keys.
Initial exchange converts those keys to X25519; temporary session keys are separate X25519 pairs.
The incoming peer uses the server role and the outgoing peer uses the client role.
Packet bodies are encrypted and decrypted using those keys.

Node startup saves the 64-byte signing secret in the configured keys file.
Existing 32-byte exchange-key files are rejected without replacement.
There is no automatic key migration.

## Messages

Incoming packets are interpreted and routed into the node.
Internal messages can request connections, disconnections, or broadcasts.
The messenger keeps state for sending work to multiple peers.

The P2P directory handles transport and peer protocol state.
Storage operations are passed to the actors responsible for them.

## Transport and routing

Each connection has separate read and write packet state.
Packet cursors allow socket processing to track progress across events.
The packet format carries length, version, key, nonce, and authentication data.
Actor and operation information identifies the work represented by a packet.

Peer identity and session keys have different roles.
The citizen record is used when deciding whether to accept a peer.
Session keys are used to protect the exchanged packet bodies.
A live connection can carry work requested by other actors.

## References

- [node/p2p/src/manager.cpp:37](../../node/p2p/src/manager.cpp#L37) sets up the TCP server.
- [node/p2p/src/conn_types.h:14](../../node/p2p/src/conn_types.h#L14) defines handshake and connection states.
- [node/p2p/src/manager.h:34](../../node/p2p/src/manager.h#L34) groups connection and peer lookup state.
- [node/p2p/src/handshake.cpp:36](../../node/p2p/src/handshake.cpp#L36) begins the incoming authorization path.
- [node/p2p/src/pkt.h:25](../../node/p2p/src/pkt.h#L25) defines the packet prefix layout.
- [node/p2p/src/handlers.cpp:163](../../node/p2p/src/handlers.cpp#L163) contains peer error handling notes.

## TODO

- Complete broadcast retry handling.
  [node/p2p/src/handlers.cpp:33](../../node/p2p/src/handlers.cpp#L33)

- Record failed or rejected peer activity where the handlers call for it.
  [node/p2p/src/handlers.cpp:142](../../node/p2p/src/handlers.cpp#L142)
  [node/p2p/src/handlers.cpp:163](../../node/p2p/src/handlers.cpp#L163)
