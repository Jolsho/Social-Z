# Client networking

The networking layer is meant to track exchanges while they are in progress.
The client should be able to request data and continue handling the interface.
A context connects the outgoing request to the parser for its eventual response.

Networking is organized around request contexts.
A ContextID refers to the state used for an exchange.
The client can associate send and receive buffers with that context.

A context can be dead, idle, sending, or receiving.
It also records which parser should handle the response.
Released contexts are put back on a free list.

## Request flow

Create a context.
Marshal the request into bytes.
Send it through the host or native transport.
Pass response bytes into client_parse_response().
Dispatch to the parser registered for that context.
Return buffers and release the context when finished.

The public API expects sending to be asynchronous.
It gives the sender ownership of the outgoing buffer.
The response parser borrows the bytes passed to it.
Buffer ownership matters because the pool reuses that memory.

The design leaves room for native and WebAssembly integration.

## Why contexts exist

Different requests can need different response parsers.
Recording a parser ID keeps the transport from needing to understand posts.
The transport only has to deliver bytes with the matching context ID.
The data layer handles what those bytes mean.

Send and receive buffers are tracked separately.
Releasing them returns their memory to the client's buffer pool.
A context can then be reused for another exchange.

## References

- [client/src/netwrk/networker.h:13](../../client/src/netwrk/networker.h#L13) defines each connection state record.
- [client/src/netwrk/networker.h:32](../../client/src/netwrk/networker.h#L32) groups states, buffers, and parsers.
- [client/src/netwrk/context.h:13](../../client/src/netwrk/context.h#L13) defines the connection states.
- [client/src/netwrk/client.c:11](../../client/src/netwrk/client.c#L11) allocates a context from the free list.
- [client/src/netwrk/client.c:30](../../client/src/netwrk/client.c#L30) releases a context and its buffers.
- [client/src/netwrk/client.c:51](../../client/src/netwrk/client.c#L51) dispatches a response to its parser.
- [client/src/netwrk/context.c:36](../../client/src/netwrk/context.c#L36) ensures a buffer has sufficient capacity.
- [client/src/netwrk/parsers.h:22](../../client/src/netwrk/parsers.h#L22) lists the data response parsers.

## TODO

- Fill in network initialization and context allocation setup.
  [client/src/netwrk/networker.h:52](../../client/src/netwrk/networker.h#L52)

- Queue outgoing requests instead of sending directly.
  [client/src/client.h:57](../../client/src/client.h#L57)

- Complete transport handling for partial responses and buffer lifetimes.
  [client/src/client.h:17](../../client/src/client.h#L17)
