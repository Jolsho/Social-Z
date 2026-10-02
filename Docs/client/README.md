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

init_client() initializes the pool, cache, feed, and networking.
destroy_client() releases those resources.
Rendering startup initializes the world separately.
Username/password login uses the existing request marshaler, send_request hook, and response parser.
See [login](login.md) for the flow and buffer ownership rules.
Login and blob response parsers are enabled while the feed format is being repaired.

## Components

### [data](data.md)

In-memory post views and packed feed storage.
Request and reply code lives under netwrk.

### codec

Client-only plaintext marshaling and parsing.
The [account header](account.md) and [feed pages](feed.md) are encoded here.
The node receives encrypted blobs and does not interpret these fields.

### [networking](networking.md)

Request contexts, connection state, and response dispatch.
marshalers/ builds outgoing requests and parsers/ handles replies.
client/src/login.c coordinates login without mixing that workflow into record codecs.
netwrk/login.c keeps the small lookup and fetch message helpers private to the client.

### [world](world.md)

Input, camera state, entities, and spatial queries.

### [rendering](rendering.md)

Rendering commands and an interface for graphics backends.

### math / utils

Geometry helpers, buffer pools, [buffer encryption](crypto.md), and cached storage.

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
- [client/src/client.c:13](../../client/src/client.c#L13) initializes core client state.
- [client/src/client.c:30](../../client/src/client.c#L30) releases owned client resources.
- [client/src/wrld/wrld.c:11](../../client/src/wrld/wrld.c#L11) initializes the world before the frame loop.
- [client/include/sz_client/client.h:43](../../client/include/sz_client/client.h#L43) describes the host sending interface.

## TODO

- Implement the missing BVH initializer before running the rendering startup path.
  [client/src/wrld/bvh/bvh.h:42](../../client/src/wrld/bvh/bvh.h#L42)

- Complete the path from feed data to visible entities.
  [client/src/wrld/wrld.c:170](../../client/src/wrld/wrld.c#L170)

- Connect the world to frame rendering.
  [client/src/wrld/wrld.c:20](../../client/src/wrld/wrld.c#L20)
