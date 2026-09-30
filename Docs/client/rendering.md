# Client rendering

The renderer is meant to draw the interface without fixing it to one graphics API.
The world should describe visible items through a common set of commands.
Platform backends should turn those commands into actual graphics operations.

Rendering is split into commands and a backend interface.
The client describes what to draw.
A backend is expected to perform the graphics work.

## Commands

A command buffer stores a sequence of rendering instructions.
These include starting a render pass and choosing a pipeline.
They also bind resources and issue draw or dispatch commands.

## Backend

The interface covers buffers, textures, shaders, and samplers.
It also covers pipelines and descriptor sets.
Resource handles let commands refer to those objects.

## Meshes

The mesh code uses cgltf to read glTF data.
A mesh records its vertex and index buffers.
It also records the counts and formats needed to draw it.

## How the abstraction fits

Resource descriptions specify what a buffer or texture should contain.
Resource handles let later commands refer to the created objects.
A pipeline describes the graphics setup used for a draw.
Descriptor sets supply the resources used by that setup.

Commands are stored as headers followed by their payloads.
This lets the client collect work before asking the backend to execute it.
The mesh loader converts glTF geometry into vertex and index buffers.
Those buffers can then be referenced by draw commands.

## References

- [client/src/render/backend.h:21](../../client/src/render/backend.h#L21) begins the backend lifecycle interface.
- [client/src/render/command.h:15](../../client/src/render/command.h#L15) defines command buffer storage.
- [client/src/render/command.h:25](../../client/src/render/command.h#L25) lists supported command opcodes.
- [client/src/render/command.h:35](../../client/src/render/command.h#L35) defines the command header.
- [client/src/render/mesh.c:231](../../client/src/render/mesh.c#L231) loads a glTF mesh.
- [client/src/render/mesh.c:317](../../client/src/render/mesh.c#L317) releases mesh resources.
- [client/src/client.c:48](../../client/src/client.c#L48) is the intended frame rendering entry point.

## TODO

- Provide graphics backends implementing the declared interface.
  [client/src/render/backend.h:21](../../client/src/render/backend.h#L21)

- Gather visible entities and build their rendering commands.
  [client/src/client.c:49](../../client/src/client.c#L49)

- Connect command execution to the host's frame presentation.
