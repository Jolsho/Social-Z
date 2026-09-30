# Node actors and logging

The actor layer is meant to give each service its own work and event state.
Services should communicate through explicit requests instead of shared internals.
This keeps networking, storage, and logging responsibilities separate.

Actors separate the node into services which communicate through messages.
The API directory implements the actor interface and central routing loop.
Public headers live under include/sz_node.

## Messages

A message records its sender and destination.
It also carries a connection ID, code, priority, and data buffer.
The code tells the receiving service which operation to perform.
Shared operation codes are defined in common/include/sz_common/paths.h.

## Queues

Each actor has incoming and outgoing channels.
Messages are split into critical, control, work, and telemetry priorities.
Queue sizes and processing budgets are configurable.
Statistics track accepted and dropped messages.

The central loop polls outgoing actor events.
It moves messages into the destination actor's incoming buffers.
It then updates the queues to publish the work.

## Utils

Utilities support channels, database access, errors, and shutdown.
Other helpers track keys, time, and reusable buffers.
Log accumulators collect messages before forwarding them to LOG.

## Log

The logging actor receives log messages from the other services.
It parses those messages and writes them to a log file.

## Processing and ownership

A service polls for input messages and available output message slots.
It handles the incoming work and fills outgoing slots with replies or requests.
Updating the actor publishes the work and advances consumed slots.

The central loop transfers the message envelope and its data reference.
Buffer ownership therefore travels with the message.
The router has a return or cleanup path when a destination has no free slot.
Reusable buffers are meant to limit allocations during steady processing.

Priorities let control traffic have a different budget from ordinary work.
Telemetry reports queue behavior so congestion can be observed.

## References

- [node/include/sz_node/actor.h:64](../../node/include/sz_node/actor.h#L64) defines queue sizes and processing budgets.
- [node/include/sz_node/actor.h:121](../../node/include/sz_node/actor.h#L121) declares polling and update operations.
- [node/include/sz_node/msgT.h:17](../../node/include/sz_node/msgT.h#L17) defines message priorities.
- [node/include/sz_node/msgT.h:24](../../node/include/sz_node/msgT.h#L24) defines the message fields.
- [node/api/sz.c:93](../../node/api/sz.c#L93) begins central message dispatch.
- [node/log/src/logger.cpp:72](../../node/log/src/logger.cpp#L72) parses incoming log records.
- [node/log/src/logger.cpp:85](../../node/log/src/logger.cpp#L85) runs the log service loop.

## TODO

- Complete runtime startup for all required actor identities.
  [node/api/sz.c:63](../../node/api/sz.c#L63)

- Keep buffer return and cleanup rules consistent between services.

- Finish shutdown handling alongside the service loops.
