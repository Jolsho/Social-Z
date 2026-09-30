# Social-Z

Social-Z is an open source project for community networks.
It aims to give communities shared tools for communication, publishing, and economic exchange.

The idea is that people should be able to build and participate in networks around their shared interests and needs.
Each community should be able to operate its own infrastructure and develop its own approach to cooperation.

## Community networks

A community network brings together people and the computers that support their shared activity.
Members could use it to share information, organize discussions, publish offers, and exchange goods or services.

Social-Z is intended to bring those activities into one system.
Its design combines a social interface, distributed content storage, peer to peer communication, and a ledger for shared economic state.
The longer term goal is to let communities choose the rules of their own exchanges while individuals can participate in multiple networks.

## Project structure

- **client** provides the user interface foundations, social data handling, input, and rendering interfaces.
- **node** provides backend services for peer communication, storage, structured records, and ledger work.
- **common** provides shared types, containers, buffers, hashing, and cryptographic helpers.
- **proxy** is intended to connect a web interface to the node through HTTP.
- **Development** contains environment management tools and alternate implementations.

The main libraries are written in C and C++ and use CMake.
The Go proxy and development daemon have separate modules.

Optional tests use CMake and CTest.
See [testing instructions](Docs/testing.md) for running all registered tests or selecting a component.

## Development status

Social-Z is an early work in progress.
The repository contains a mixture of implemented components, partial integrations, and design sketches.
It does not yet provide a complete community network application.

Current work includes connecting the client and backend, completing rendering and request handling, and revising the ledger design.
The node's event handling currently depends on Linux facilities.

## Documentation

Start with the [project overview](Docs/README.md).
The documentation uses short explanations and links to the relevant source files.

- [Client](Docs/client/README.md)
- [Node](Docs/node/README.md)
- [Shared utilities](Docs/common/README.md)
- [Proxy](Docs/proxy/README.md)
- [Development tools](Docs/Development/README.md)

Each documentation directory follows the corresponding source directory.
Component pages explain the intended design and list unfinished work in TODO sections.

The [Notes](Notes) directory is kept for personal thoughts, sketches, and exploratory designs.
It includes the existing licensing, modeling, and ledger notes.

## Contributing

Before contributing, read the [Contributor License Agreement](CLA.md).
Then add your own JSON file to [.contributors](.contributors), named after your GitHub username.
Use [jolsho.json](.contributors/jolsho.json) as an example.

Include your GitHub username, the CLA version you accept, and the date of acceptance in `YYYY-MM-DD` format.
Adding this record confirms that you have read and agree to the CLA.
Commit your contributor record before making regular contribution commits.
You only need to add yourself once for the current CLA version.

The [documentation](Docs/README.md) explains the project structure and includes TODO sections to help identify work.

## License

Copyright (c) 2026 Joshua Olson

This project is licensed under the GNU Lesser General Public License, version 3.0 or later.
See [LICENSE](LICENSE) for the full license text.
