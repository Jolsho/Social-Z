#!/bin/bash
set -e

DEPS=./dep_src
mkdir -p "$DEPS"

LIB_DST="$(pwd)/lib"
mkdir -p "$LIB_DST"

INCLUDE="$LIB_DST/include"
mkdir -p "$INCLUDE"

# libSodium
SODIUM=libsodium-stable
LIB_SODIUM=libsodium
SODIUM_VERSION=1.0.21
SODIUM_URL=https://download.libsodium.org/libsodium/releases/libsodium-$SODIUM_VERSION-stable.tar.gz 
if [[ ! -d "$LIB_DST/$LIB_SODIUM" ]]; then
    mkdir -p "$LIB_DST/$LIB_SODIUM"
    TAR=sodium.tar.xz 
    (

        cd "$DEPS"
        if [ ! -d "$SODIUM" ]; then
            echo "Downloading libSodium v$SODIUM_VERSION..."
            curl -L -o "$TAR" "$SODIUM_URL"
            tar -xf $TAR
            rm $TAR
        fi

        cd "$SODIUM"
        ./configure --prefix="$LIB_DST/$LIB_SODIUM"
        make -j2 && make check
        make install
    )
fi

# lmdb
LMDB=lmdb
LMDB_URL=https://github.com/LMDB/lmdb.git
if [[ ! -d "$LIB_DST/$LMDB" ]]; then
    mkdir -p "$LIB_DST/$LMDB"

    if [ ! -d "$DEPS/$LMDB" ]; then
        echo "Downloading liblmdb..."
        git clone "$LMDB_URL" "$DEPS/$LMDB"
    fi
    (
        echo "Building liblmdb..."
        cd "$DEPS/$LMDB/libraries/liblmdb"
        make -j2
        cp liblmdb.a "$LIB_DST/$LMDB"
        cp lmdb.h "$INCLUDE"
    )
fi

# blake3
BLAKE3="blake3"
BLAKE3_URL=https://github.com/BLAKE3-team/BLAKE3.git
if [ ! -d "$LIB_DST/$BLAKE3" ]; then
    mkdir -p "$LIB_DST/$BLAKE3"


    if [ ! -d "$DEPS/$BLAKE3" ]; then
        echo "Downloading blake3..."
        git clone "$BLAKE3_URL" "$DEPS/$BLAKE3"
    fi
    # TODO -- checkout v1.8.3
    (
        echo "Building blake3..."
        cd "$DEPS/$BLAKE3/c"
        cmake -B build
        cmake --build build
        cp build/libblake3.a "$LIB_DST/$BLAKE3"
        cp blake3.h "$INCLUDE"
    )
fi

# openssl
SSL="openssl"
SSL_URL=https://github.com/openssl/openssl/releases/download/openssl-3.5.5/openssl-3.5.5.tar.gz
if [ ! -d "$LIB_DST/$SSL" ]; then
    mkdir -p "$LIB_DST/$SSL"

    if [ ! -d "$DEPS/$SSL" ]; then
        echo "Downloading openssl..."
        curl -L  "$SSL_URL" -o "$DEPS/$SSL.tar.gz"
    fi
    (
        cd "$DEPS"
        mkdir "$SSL"
        tar -xzf "$SSL.tar.gz" --strip-components=1 -C "$SSL"
        rm "$SSL.tar.gz"


        echo "Building openssl..."
        cd "$SSL"
        ./Configure linux-x86_64 \
            --prefix="$LIB_DST/$SSL" \
            no-apps \
            no-tests \
            no-docs \
            no-shared \
            no-legacy \
            no-comp \
            no-engine \
            no-ssl3 \
            no-tls1 \
            no-tls1_1 \
            no-dso

        make -j2
        make install_sw
    )
fi

# llhttp
HTTP="llhttp"
HTTPV="v9.3.1"
HTTP_URL=https://github.com/nodejs/llhttp/archive/refs/tags/release/$HTTPV.tar.gz
if [ ! -d "$LIB_DST/$HTTP" ]; then
    mkdir -p "$LIB_DST/$HTTP"

    echo "Downloading llhttp..."
    curl -L  "$HTTP_URL" -o "$DEPS/$HTTP.tar.gz"
    (
        cd "$DEPS"
        mkdir "$HTTP"
        tar -xzf "$HTTP.tar.gz" --strip-components=1 -C "$HTTP"
        rm "$HTTP.tar.gz"


        echo "Building llhttp..."
        cd "$HTTP"
        cmake -B build \
            -DLLHTTP_BUILD_SHARED_LIBS=Off \
            -DLLHTTP_BUILD_STATIC_LIBS=On
        cmake --build build
        cp include/llhttp.h "$INCLUDE"
        cp build/libllhttp.a "$LIB_DST/$HTTP"
    )
fi


# sqlite3
SQLITE="sqlite"
SQLITEV="3510300"
SQL_URL="https://sqlite.org/2026/sqlite-amalgamation-$SQLITEV.zip"
if [ ! -d "$LIB_DST/$SQLITE" ]; then
    mkdir -p "$LIB_DST/$SQLITE"

    echo "Downloading SQLite..."
    curl -L "$SQL_URL" -o "$DEPS/$SQLITE.zip"

    (
        cd "$DEPS"
        mkdir "$SQLITE"
        unzip "$SQLITE.zip" -d "$SQLITE"

        cd "$SQLITE/sqlite-amalgamation-$SQLITEV"

        cp sqlite3.c "$LIB_DST/$SQLITE"
        cp sqlite3.h "$INCLUDE"
        cp sqlite3ext.h "$INCLUDE"
    )
fi

# simdjson
JSON="simdjson"
JSON_URL_ROOT="https://raw.githubusercontent.com/simdjson/simdjson/master/singleheader"
if [ ! -d "$LIB_DST/$JSON" ]; then
    mkdir -p "$LIB_DST/$JSON"
    (
        echo "Downloading simdjson..."
        cd "$LIB_DST/$JSON"
        wget "$JSON_URL_ROOT/simdjson.h"
        wget "$JSON_URL_ROOT/simdjson.cpp"
        mv simdjson.h "$INCLUDE"
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

