#!/bin/bash

set -ex

# Must be provided by user
RELEASE_DIR=$(realpath $1)
UPSTREAM_DIR="$RELEASE_DIR/upstream/"

nJOBS=$(getconf _NPROCESSORS_ONLN)

cd $UPSTREAM_DIR

build_qemu() {
  cd $UPSTREAM_DIR/qemu; rm -fr build; mkdir build; cd build; ../configure --enable-avx2 --enable-avx512f --target-list=x86_64-softmmu
  make -j${nJOBS}
}

build_dramsim3() {
  DRAMsim3_DIR=$UPSTREAM_DIR/DRAMsim3
  cd $DRAMsim3_DIR/pnm_sim; rm -fr build; mkdir build; cd build; cmake .. -DCMD_TRACE=1; make -j${nJOBS}
  # Enable address trace: cmake .. -DCMD_TRACE=1 -DADDR_TRACE=1
  cd $DRAMsim3_DIR/trace_gen; scons
}

# 1. Build qemu and DRAMSim3 for performance simulator.
(build_qemu)
(build_dramsim3)
