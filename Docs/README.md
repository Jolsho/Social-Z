# Social-Z documentation

The general idea is to give a community network the software for its own economy.
Communication gives people a way to organize around that economy.
Shared records and storage give that communication somewhere to live.
The ledger is intended to supply the shared economic state.

Social-Z is about people choosing how they cooperate within community networks.
People should be able to form and leave their own economic networks.
These groups are described here as community networks.
The software is meant to support communication and exchange within them.

## Project structure

### [common](common/README.md)

Shared types and utilities used by the C and C++ code.
Includes buffers, containers, hashing, and cryptography.

### [client](client/README.md)

The user side of the system.
Holds social data, network state, input, and the visual world.

### [node](node/README.md)

The backend which stores data and communicates with peers.
Its components exchange messages through actors.

### [proxy](proxy/README.md)

A Go HTTP server and bridge into the node.
Meant to serve a web interface and forward requests.

### [Development](Development/README.md)

Tools and experiments around running the system.
Includes a daemon and a separate C++ proxy implementation.

The root CMake project builds common, node, and client.
The Go programs have their own modules.

## Reading the documentation

Each documentation directory corresponds to a source directory in the project.
README.md gives the general picture of that directory.
The other files describe its main components.

[Notes](../Notes) is kept separately for personal thoughts, sketches, and exploratory designs.
Those files can describe possibilities beyond the current implementation.

## How the pieces fit

The client turns stored social data into something a person can use.
The node provides the services behind that data.
P2P lets nodes exchange messages without a central HTTP server.
The proxy is intended to expose node services to a web interface.
Development tools deal with the environment around these programs.

The split lets the visual interface and backend develop separately.
Common provides the types they need to agree on.
Actor messages provide the internal boundary between node services.

## References

Source reference labels start from the project root.
A reference has the form `directory/file.ext:111`.
The number points to the relevant line in the current source.
Links open the corresponding source file.

- [README.md:9](../README.md#L9) introduces community networks.
- [CMakeLists.txt:13](../CMakeLists.txt#L13) starts the main C and C++ build structure.
- [client/src/client.h:30](../client/src/client.h#L30) brings the client components together.
- [node/main.cpp:14](../node/main.cpp#L14) shows the standalone node startup.
- [proxy/main.go:14](../proxy/main.go#L14) connects the Go server and node bridge.

## TODO

- Connect the client, proxy, and node into a complete request flow.

- Finish the service wiring and the platform integrations.

The component pages describe the unfinished pieces in more detail.
