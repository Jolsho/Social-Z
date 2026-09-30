# Development daemon

The daemon is meant to manage the machine supporting a community network node.
It should give the operator one interface for the surrounding services.
Container and firewall details live behind that interface.

The daemon is intended to manage the environment around the node.
It has its own Go module.
It uses a PID file to track the background process.

## Cli

The command interface sends JSON messages over a local Unix socket.
Commands cover production, network changes, tests, and restarts.
There are also helpers for rebuilding the frontend and backend.

## Docker

Container models describe Social-Z, PostgreSQL, and Ethereum services.
The manager can arrange production or test environments.
Test requests include counts for the different services.
Utility code supports networks, logging, and message formats.

## Firewall

The firewall code uses Linux nftables.
It keeps sets of citizen addresses and banned addresses.
Socket requests can add citizens or change bans.
There is also setup code for SSH rules.

## Router

The router package is intended to manage the local networking environment.

## Control flow

The entry point checks whether a daemon process already exists.
The CLI translates commands into codes and JSON payloads.
A Unix socket carries those messages to the appropriate manager.

The Docker manager dispatches requests through a Session.
Its service models hold the state for the different container groups.
Production and test requests arrange the backend network and its services.
Test counts let an operator request more than one instance of a service.

Firewall management has a separate socket and session state.
It changes address sets used by nftables rules.
Persisted addresses are meant to survive a daemon restart.

## References

- [Development/daemon/main.go:58](../../Development/daemon/main.go#L58) checks the PID file and process state.
- [Development/daemon/daemon.go:25](../../Development/daemon/daemon.go#L25) launches the background process.
- [Development/daemon/cli/helpers.go:18](../../Development/daemon/cli/helpers.go#L18) maps commands to message codes.
- [Development/daemon/docker/entry.go:24](../../Development/daemon/docker/entry.go#L24) starts the manager socket.
- [Development/daemon/docker/models/main.go:31](../../Development/daemon/docker/models/main.go#L31) dispatches Docker requests.
- [Development/daemon/docker/models/main.go:44](../../Development/daemon/docker/models/main.go#L44) handles test service counts.
- [Development/daemon/firewall/socket.go:68](../../Development/daemon/firewall/socket.go#L68) dispatches firewall requests.
- [Development/daemon/firewall/citizens.go:27](../../Development/daemon/firewall/citizens.go#L27) adds citizen addresses.

## TODO

- Complete coordinated shutdown and restart handling.
  [Development/daemon/main.go:21](../../Development/daemon/main.go#L21)

- Implement the router manager.
  [Development/daemon/router/main.go:14](../../Development/daemon/router/main.go#L14)

- Revisit firewall persistence as suggested by the source notes.
  [Development/daemon/firewall/database.go:21](../../Development/daemon/firewall/database.go#L21)
