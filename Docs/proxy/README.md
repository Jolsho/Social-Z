# Web proxy

The proxy is meant to give a browser access to node services.
It translates between HTTP exchanges and the node's actor messages.
It also serves the web interface that makes those services usable.

The proxy is a Go server which bridges HTTP and the node.
It uses cgo to call the node's C interface.
It is separate from the root CMake build.

### server

Configures the HTTP routes and static file server.
There are routes for social requests and administration.
The configured server uses TLS on the local machine.

### sz

Wraps node actors, messages, and byte vectors for Go.
An RPC loop exchanges messages with the HTTP side through channels.
A buffer store is intended to reuse node data buffers.

The intended flow starts with an HTTP request.
The proxy turns it into a node message.
The node response is then forwarded to the waiting HTTP request.

## Request and response lifetimes

The HTTP server keeps channels to and from the RPC bridge.
Its waiting map is intended to associate request IDs with response channels.
This gives a node reply a path back to the HTTP request waiting for it.

The RPC bridge polls an actor in the node runtime.
It wraps incoming C messages for the Go server.
Requests from Go are placed into available outgoing node message slots.
Vector wrappers expose the underlying C bytes without a separate content format.
The buffer store is intended to reuse those allocations after an exchange.

## References

- [proxy/main.go:14](../../proxy/main.go#L14) creates the channels and starts both sides.
- [proxy/server/server.go:16](../../proxy/server/server.go#L16) defines the server and waiting response map.
- [proxy/server/server.go:45](../../proxy/server/server.go#L45) begins route registration.
- [proxy/server/server.go:65](../../proxy/server/server.go#L65) starts the TLS listener.
- [proxy/sz/rpc.go:27](../../proxy/sz/rpc.go#L27) starts the embedded node actors.
- [proxy/sz/rpc.go:63](../../proxy/sz/rpc.go#L63) runs the Go and C message bridge.
- [proxy/sz/vector.go:19](../../proxy/sz/vector.go#L19) wraps a C vector for Go.

## TODO

- Marshal HTTP requests and forward their replies.
  [proxy/server/social.go:23](../../proxy/server/social.go#L23)
  [proxy/server/admin.go:23](../../proxy/server/admin.go#L23)

- Implement the intended security and rate limit checks.
  [proxy/server/social.go:14](../../proxy/server/social.go#L14)
  [proxy/server/server.go:24](../../proxy/server/server.go#L24)

- Align C bindings with the current node headers and actor lifecycle.
  [proxy/sz/rpc.go:13](../../proxy/sz/rpc.go#L13)
