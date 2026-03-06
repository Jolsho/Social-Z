#!/bin/bash
set -e

DEPS=./deps

# libSodium
SODIUM=libsodium-stable
SODIUM_VERSION=1.0.21
if [[ ! -d "$DEPS/$SODIUM" ]]; then
    TAR=sodium.tar.xz 
    (
        echo "Downloading libSodium v$SODIUM_VERSION..."
        cd "$DEPS"
        curl -L -o "$TAR" https://download.libsodium.org/libsodium/releases/libsodium-$SODIUM_VERSION-stable.tar.gz 
        tar -xf $TAR
        rm $TAR

        cd "$SODIUM"
        ./configure
        make && make check
        sudo make install
    )
fi


export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
if [[ ! -d "./build" ]]; then 
    cmake -B build
fi

cmake --build build

RUN=0
while getopts "r" opt; do
  case $opt in
    r) RUN=1 ;;
    *) echo "Usage: $0 [-r]"; exit 1 ;;
  esac
done

if [[ $RUN -eq 1 ]]; then
    ./build/sz
fi

