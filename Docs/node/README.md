# Node

The node is supposed to let a computer participate in a community's network.
It provides local storage and exchanges data with other participants.
Its services are intended to cooperate without becoming one large processing loop.

The node is the backend of Social-Z.
It is written in C and C++.
It can be built as the sz_node library or the socialz executable.
The event handling code currently uses Linux epoll.

The node is divided into actors.
Each actor has queues for incoming and outgoing messages.
The central loop routes messages between them.
The services can poll their own events and run on separate threads.

## Components

### [p2p](p2p.md)

TCP connections and encrypted communication with peers.

### [storage](storage.md)

Blob storage, transfer sessions, permissions, and vouchers.

### [database](database.md)

Structured records and database request handling.

### ledger

The existing ledger code contains state trees and commitment proofs.
The [ledger notes](../../Notes/node/ledger.txt) describe its planned rewrite.

### [API, utilities, and logging](actors.md)

Actor interfaces, message routing, shared helpers, and logging.

The node does have a TCP listener through the P2P component.
The HTTP interface is handled by the separate proxy code.

## How the services cooperate

P2P deals with the remote connection and packet handling.
FS deals with content transfers and permission checks.
DB deals with structured records and queries.
LOG receives diagnostic messages from those services.
The ledger is intended to manage shared economic state.

A service sends a message when it needs another service to do work.
The central router transfers that message to the destination actor.
Replies can use the connection ID to identify the original exchange.

## References

- [node/main.cpp:14](../../node/main.cpp#L14) creates the runtime and starts service threads.
- [node/api/sz.c:40](../../node/api/sz.c#L40) registers an actor with the runtime.
- [node/api/sz.c:56](../../node/api/sz.c#L56) runs the central message router.
- [node/include/sz_node/actor.h:54](../../node/include/sz_node/actor.h#L54) lists actor identities.
- [node/include/sz_node/msgT.h:24](../../node/include/sz_node/msgT.h#L24) defines the message envelope.

## TODO

- Complete actor registration and startup for the chosen runtime.
  [node/main.cpp:14](../../node/main.cpp#L14)
  [node/api/sz.c:63](../../node/api/sz.c#L63)

- Connect the ledger actor to its processing loop.
  [node/main.cpp:26](../../node/main.cpp#L26)

- Align request formats across the services and proxy.
