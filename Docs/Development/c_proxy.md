# C++ proxy experiment

This proxy is meant to expose node services through a C++ HTTP server.
It explores keeping that boundary within the node's actor infrastructure.
Connections should become requests to actors and receive responses back.

This is a separate C++ implementation of an HTTP proxy.
It has its own CMake file.
The server uses OpenSSL for TLS and epoll for socket events.

Connections keep buffers and state while requests arrive.
The parser reads HTTP request information.
The server builds responses and writes them back to the connection.
There is also code for parsing WebSocket frames.

This is a separate implementation from the Go code under proxy.

## Connection and parsing flow

The server accepts a socket and establishes its TLS connection state.
Epoll reports when the connection can be read or written.
HTTP parser callbacks collect methods, URLs, headers, and body bytes.
The server can then build and send a response for the connection.

Parsing is separated from the socket event loop.
That lets protocol fields be collected across multiple reads.
WebSocket parsing provides another possible way to carry ongoing exchanges.
Actor message handling is the intended link to backend work.

## References

- [Development/c_proxy/server.cpp:57](../../Development/c_proxy/server.cpp#L57) sets up the server.
- [Development/c_proxy/server.cpp:151](../../Development/c_proxy/server.cpp#L151) runs the socket event loop.
- [Development/c_proxy/server.cpp:272](../../Development/c_proxy/server.cpp#L272) accepts new connections.
- [Development/c_proxy/parse.cpp:34](../../Development/c_proxy/parse.cpp#L34) collects URL data through parser callbacks.
- [Development/c_proxy/parse.cpp:259](../../Development/c_proxy/parse.cpp#L259) collects request body data.
- [Development/c_proxy/ws.cpp:9](../../Development/c_proxy/ws.cpp#L9) parses WebSocket frames.
- [Development/c_proxy/server.cpp:347](../../Development/c_proxy/server.cpp#L347) builds outgoing responses.

## TODO

- Connect actor replies to the waiting HTTP connections.
  [Development/c_proxy/server.cpp:345](../../Development/c_proxy/server.cpp#L345)

- Implement error handling for backend messages.
  [Development/c_proxy/server.cpp:344](../../Development/c_proxy/server.cpp#L344)

- Complete request dispatch and the remaining response paths.
  [Development/c_proxy/server.cpp:392](../../Development/c_proxy/server.cpp#L392)
