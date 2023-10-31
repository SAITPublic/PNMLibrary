#!/bin/bash

set -ex

# Must be provided by user
RELEASE_DIR=$(realpath $1)
UPSTREAM_DIR="$RELEASE_DIR/upstream/"

cd $UPSTREAM_DIR

build_linux() {
  cd $UPSTREAM_DIR/linux
  rm -fr debian/ vmlinux-gdb.py
  rm -f .config; cp -v config-pnm .config
  rm -fr ../linux*.deb ../linux-upstream*.buildinfo ../linux-upstream*.changes

  # Do linux build.
  make CC="ccache gcc" -j `getconf _NPROCESSORS_ONLN` bindeb-pkg

  # Install deb packages.
  sudo dpkg -i ../linux*.deb

  echo "PNM linux kernel is built and installed, reboot your system and proceed with ./build_userspace.sh $RELEASE_DIR"
}

# Build and install linux.
(build_linux)
