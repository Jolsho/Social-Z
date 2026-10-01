# Running tests

Project tests use CMake and CTest.
You need CMake 3.21 or later and a C and C++ compiler.
Run the following commands from the project root.

```sh
cmake --preset tests
cmake --build --preset tests
ctest --preset tests
```

The tests preset configures a separate build under `build/tests`.
It skips application setup and builds the registered test executables.
The hash test downloads the project's pinned BLAKE3 dependency on first configuration.
The crypto test builds libsodium inside the test build directory on its first build.
Building libsodium requires Make and a shell; Windows users can use WSL.
Other application dependencies are not required.
Rebuild before running tests after changing code.
CTest runs the executables that have already been built.

## Selecting a component

```sh
ctest --preset tests -L '^common$'
ctest --preset tests -L '^client$'
ctest --preset tests -L '^node$'
```

Tests are currently registered for common and client.
Node has no registered tests yet.
Use `ctest --preset tests -N` to list the registered tests after configuring.
An empty selection reports an error rather than a successful test run.
The existing ledger tests are not included in this setup.

The store eviction test prepares its own table storage to isolate eviction behavior.
Store setup and cleanup are covered separately by the client store lifecycle test.
That test also covers buffer ownership, replacement, and failed queue growth.
The common hash table storage test exercises the real allocator, alignment, and cleanup.
The hash test checks published BLAKE3 vectors, incremental updates, and independent state copies.
The crypto test covers signatures, tampering, signing-key conversion, and both exchange directions.
It does not exercise the complete P2P handshake or node key-file loading.
The blob test assembles chunks, verifies the final hash, and covers partial-read and cleanup failures.
Context tests use real networking setup and exercise allocation failures and shutdown.
The client lifecycle test uses the public constructor and destructor with real blob parsing.
It also forces each startup allocation to fail and checks cleanup.

The login test uses a host implementation of send_request with real marshaling, parsing, and crypto.
It covers chunked retrieval, wrong passwords, identity binding, malformed records, and cancellation.
It overwrites borrowed replies and checks deferred buffer returns and context reuse.
It also covers pool exhaustion, synchronous replies, and password re-encryption with stable keys.
Node username registration and real proxy/P2P transport remain unverified.

## Memory checks

For GCC or Clang builds, use the sanitizer preset.
The compiler and platform must support AddressSanitizer and UndefinedBehaviorSanitizer.

```sh
cmake --preset tests-sanitized
cmake --build --preset tests-sanitized
ctest --preset tests-sanitized
```

This uses a separate build under `build/tests-sanitized`.
The sanitizer option applies only to test targets linked to `sz_test_options`.

## Adding a test

Keep test sources in `common/tests`, `client/tests`, or `node/tests`.
Each component registers its executables in its own `tests/CMakeLists.txt`.
The shared setup discovers those component CMake files when they exist.

Use [the queue test registration](../common/tests/CMakeLists.txt) as an example.
Register a test with `sz_add_test(component name sources...)`.
For example, `sz_add_test(common pqueue pqueue.c ../src/structs/pqueue.c)`.
The shared helper supplies include paths, C11 settings, test options, and the component label.
Add any test-specific libraries or settings after that call.
A failed test must return a nonzero exit status.
The shared options keep assertions enabled in release configurations as well.

Tests using the tests-only configuration must explicitly include their required sources and dependencies.
The normal application library targets are not created in that configuration.
Portable tests can run on different platforms.
Tests of Linux-specific services will need a Linux environment.

## Normal builds

Tests are disabled by default.
Use `BUILD_TESTING=ON` to include them in a normal application configuration.
The presets set `SZ_TESTS_ONLY=ON` to skip that application configuration.
