emcmake cmake -B wasm_build \
    -DCMAKE_INSTALL_PREFIX=./wasm_install \
    -DCMAKE_BUILD_TYPE=Release

cmake --build wasm_build
