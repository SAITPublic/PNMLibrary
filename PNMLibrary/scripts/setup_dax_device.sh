#!/bin/sh

set -e

case $1 in
  "") BUILD_DIR=$(dirname $0)/../build ;;
  *) BUILD_DIR="$1" ;;
esac

if ! test -d $BUILD_DIR; then
  echo >&2 "Build directory does not exist"
  exit 1
fi

sudo "$BUILD_DIR/tools/pnm_ctl" setup-dax-device
