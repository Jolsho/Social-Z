# Development tools

These tools are meant to make a complete Social-Z environment easier to run.
They cover deployment experiments and supporting machine configuration.
They also provide space to explore alternate ways of exposing the node.

Development contains tooling and alternate implementations around Social-Z.
These programs are separate from the main client and node libraries.

### [daemon](daemon.md)

A Go daemon for managing a local environment.
Includes a CLI, Docker management, and firewall management.

### [c_proxy](c_proxy.md)

A C++ HTTP server experiment using OpenSSL and epoll.
Includes connection handling, request parsing, and WebSocket parsing.

## How to read this directory

The daemon works at the level of processes, containers, and network rules.
Its test setup describes several cooperating services on the same machine.
The C++ proxy works at the boundary between a client request and a node actor.

These are separate programs with their own setup assumptions.
The root CMake project does not start or configure them.

## References

- [Development/daemon/main.go:26](../../Development/daemon/main.go#L26) selects CLI or daemon startup.
- [Development/daemon/docker/models/main.go:44](../../Development/daemon/docker/models/main.go#L44) arranges a test environment.
- [Development/c_proxy/main.cpp:10](../../Development/c_proxy/main.cpp#L10) is the alternate proxy entry point.

## TODO

- Align the environment assumptions with the current project layout.

- Complete the request bridge in the alternate proxy.
  [Development/c_proxy/server.cpp:345](../../Development/c_proxy/server.cpp#L345)
