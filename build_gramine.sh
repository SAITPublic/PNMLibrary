#!/bin/bash

set -ex

# Must be provided by user
RELEASE_DIR=$(realpath $1)
UPSTREAM_DIR="$RELEASE_DIR/upstream/"

cd $UPSTREAM_DIR

build_gramine() {
  cd $UPSTREAM_DIR/gramine
  meson setup build/ --buildtype=release -Ddirect=enabled -Dsgx=enabled -Dsgx_driver=upstream
  sudo ninja -C build/ install
}

# Build gramine with PNM support.
(build_gramine)
