# Copyright (c) 2026 Jolsho
# SPDX-License-Identifier: LGPL-3.0-or-later

emcmake cmake -B wasm_build \
    -DCMAKE_INSTALL_PREFIX=./wasm_install \
    -DCMAKE_BUILD_TYPE=Release

cmake --build wasm_build -j3
cmake --install wasm_build

mkdir -p wasm_dst

emcc \
  -o ./wasm_dst/sz.js \
  -O3 \
  -s WASM=1 \
  -s MODULARIZE=1 \
  -s EXPORT_ES6=1 \
  -s ENVIRONMENT=node,web \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INITIAL_MEMORY=16MB \
  -s EXPORTED_FUNCTIONS="['_malloc','_free','_new_ctx','_decrypt','_encrypt']" \
  -s EXPORTED_RUNTIME_METHODS="[]" \
  -s ASSERTIONS=0 \
  -I$(pwd)/wasm_install/include \
  -L$(pwd)/wasm_install/lib \
  -lsz_client \
  -I$(pwd)/wasm_build/_deps/sodium/include \
  -L$(pwd)/wasm_build/_deps/sodium/lib \
  -lsodium \
  -I$(pwd)/wasm_build/_deps/blake3/src/c \
  -L$(pwd)/wasm_build/_deps/blake3/build \
  -lblake3
