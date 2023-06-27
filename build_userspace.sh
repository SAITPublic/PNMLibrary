#!/bin/bash

set -ex

# Must be provided by user
RELEASE_DIR=$(realpath $1)
UPSTREAM_DIR="$RELEASE_DIR/upstream/"

nJOBS=$(getconf _NPROCESSORS_ONLN)

cd $UPSTREAM_DIR

build_pnm_library() {
  cd $RELEASE_DIR/PNMLibrary && mkdir -p test_tables && ./scripts/build.sh -cb -j ${nJOBS} -r --fsim --tests --test_tables_root test_tables
}

build_pytorch() {
  cd $UPSTREAM_DIR/pytorch
  git submodule init
  git submodule update --init --recursive
  rm -fr build
  CMAKE_C_COMPILER_LAUNCHER=ccache CMAKE_CXX_COMPILER_LAUNCHER=ccache CFLAGS="-g -fno-omit-frame-pointer" BLAS=OpenBLAS BUILD_TEST=0 BUILD_CAFFE2=1 USE_CUDA=0 USE_PNM=1 \
  PNM_INSTALL_DIR=$RELEASE_DIR/PNMLibrary/build/ python3 ./setup.py install --user
}

# 1. Build PNMLibrary in functional simulator mode.
(build_pnm_library)

# 2. Build pytorch with PNM support.
(build_pytorch)
