# Client

The client is supposed to make the community's data usable to a person.
It turns posts and content into an interactive visual interface.
It should handle data retrieval without tying that interface to one platform.

The client is the user side of Social-Z.
It is written in C and built as the sz_client library.
Its public interface is in include/sz_client/client.h.

The Client structure brings the main pieces together.
It holds user keys, a post feed, a blob store, and reusable buffers.
It also holds network state and the visual world.

## Components

### [data](data.md)

The formats and parsing code for posts, user data, and blobs.

### [networking](networking.md)

Request contexts, connection state, and response dispatch.

### [world](world.md)

Input, camera state, entities, and spatial queries.

### [rendering](rendering.md)

Rendering commands and an interface for graphics backends.

### math / utils

Geometry helpers, buffer pools, cryptography, and cached storage.

The intended frame loop updates the world and then renders it.
A host application supplies input and the platform integration.
The public API also sketches integration with JavaScript.

## How the pieces fit

Network responses first become data in the feed or blob store.
The world decides which items need interactive entities.
Rendering then turns the visible entities into graphics work.
The next input update can change focus, scroll position, or view state.

This separates the data a person has from the data currently on screen.
It also leaves sending and graphics integration to the host platform.

## References

- [client/src/client.h:30](../../client/src/client.h#L30) defines the shared Client state.
- [client/src/client.c:12](../../client/src/client.c#L12) sets up the buffer pool, blob store, and world.
- [client/src/client.c:39](../../client/src/client.c#L39) sketches the update and render loop.
- [client/include/sz_client/client.h:43](../../client/include/sz_client/client.h#L43) describes the host sending interface.

## TODO

- Initialize the network contexts and parser state.
  [client/src/netwrk/networker.h:52](../../client/src/netwrk/networker.h#L52)

- Complete the path from feed data to visible entities.
  [client/src/wrld/wrld.c:164](../../client/src/wrld/wrld.c#L164)

- Connect the world to frame rendering.
  [client/src/client.c:48](../../client/src/client.c#L48)
