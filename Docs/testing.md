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
It skips application dependencies and builds the registered test executables.
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
Link the target to `sz_test_options` and register it with `add_test()`.
Give it the matching component label with `set_tests_properties()`.
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
