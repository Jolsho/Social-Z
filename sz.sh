#!/bin/bash
set -e

DEPS=./dep_src
mkdir -p "$DEPS"

LIB_DST="$(pwd)/lib"
mkdir -p "$LIB_DST"

# libSodium
SODIUM=libsodium-stable
LIB_SODIUM=libsodium
SODIUM_VERSION=1.0.21
if [[ ! -d "$LIB_DST/$LIB_SODIUM" ]]; then
    mkdir -p "$LIB_DST/$LIB_SODIUM"
    TAR=sodium.tar.xz 
    (
        echo "Downloading libSodium v$SODIUM_VERSION..."
        cd "$DEPS"
        curl -L -o "$TAR" https://download.libsodium.org/libsodium/releases/libsodium-$SODIUM_VERSION-stable.tar.gz 
        tar -xf $TAR
        rm $TAR

        cd "$SODIUM"
        ./configure --prefix="$LIB_DST/$LIB_SODIUM"
        make && make check
        make install
    )
fi

# lmdb
LMDB=lmdb
if [[ ! -d "$LIB_DST/$LMDB" ]]; then
    mkdir -p "$LIB_DST/$LMDB/lib"
    mkdir -p "$LIB_DST/$LMDB/include"
    (
        echo "Downloading liblmdb..."
        git clone https://github.com/LMDB/lmdb.git "$DEPS/$LMDB"

        echo "Building liblmdb..."
        cd "$DEPS/$LMDB/libraries/liblmdb"
        make
        cp liblmdb.a "$LIB_DST/$LMDB/lib"
        cp lmdb.h "$LIB_DST/$LMDB/include"
    )
fi


# CLEAN UP DEPS SOURCE DIR AFTER BUILDING
if [[ -d "$DEPS" ]]; then
    rm -rf "$DEPS"
fi


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

