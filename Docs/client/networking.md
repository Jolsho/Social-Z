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

## Files

networking/ holds contexts, networking buffers, and response dispatch.
operations/login.c groups username request preparation, sending, and reply handling.
common/src/requests/requests.c holds the username request and response metadata byte formats.
codec/blob.* currently assembles encrypted transfer chunks in client-owned storage.
codec/ also handles account, feed-page, and package plaintext after decryption.
networking/dispatch.c contains the response handler table; dispatch.h declares it.
networking/client.c handles returned send buffers without triggering login requests.
Requests share a version and RequestKind prefix and use parse_request().
Account retrieval is the special case that resolves a username.
Generic blob operations will retrieve, insert, and delete all other content.
Feeds, packages, voucher lists, and permission lists share those operations.
Their contents are decoded locally by the client.

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
The sender returns the original outgoing buffer through client_return_buffer().
The descriptor remains in the context send-buffer array.
Incoming bytes may disappear after parsing returns, so retained data is copied.
Login cancellation wipes its password immediately.
Context reuse waits until any host-held request buffer is returned.

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

- [client/src/networking/networker.h:13](../../client/src/networking/networker.h#L13) defines each connection state record.
- [client/src/networking/networker.h:32](../../client/src/networking/networker.h#L32) groups states, buffers, and parsers.
- [client/src/networking/context.h:13](../../client/src/networking/context.h#L13) defines the connection states.
- [client/src/networking/client.c:44](../../client/src/networking/client.c#L44) allocates a context from the free list.
- [client/src/networking/client.c:56](../../client/src/networking/client.c#L56) releases a context and its buffers.
- [client/src/networking/client.c:94](../../client/src/networking/client.c#L94) dispatches a response to its parser.
- [client/src/networking/context.c:36](../../client/src/networking/context.c#L36) ensures a buffer has sufficient capacity.
- [client/src/networking/dispatch.c](../../client/src/networking/dispatch.c) registers internal response handlers.

## TODO

- Queue outgoing requests instead of sending directly.
  [client/src/client.h:65](../../client/src/client.h#L65)

- Complete transport handling for partial responses and buffer lifetimes.
  [client/src/client.h:17](../../client/src/client.h#L17)
