#!/bin/sh

set -ex

LIBPNM_ROOT_DIR="$(pwd)/../../"

test -n "$CC" || CC=clang++

CXXFLAGS='-std=c++17 -g'
CXXFLAGS="$CXXFLAGS -O2"
#CXXFLAGS="$CXXFLAGS -fsanitize=address,undefined"

$CC main.cpp $CXXFLAGS -I $LIBPNM_ROOT_DIR -L $LIBPNM_ROOT_DIR/build/secure -lPnmAISecure -lcrypto -o main.x
