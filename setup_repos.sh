#!/bin/bash

set -ex

# Must be provided by user
RELEASE_DIR=$(realpath $1)
PATCHES_DIR="$RELEASE_DIR/patches/"
UPSTREAM_DIR="$RELEASE_DIR/upstream/"

DEFAULT_DEPTH="100"
NO_DEPTH="-1"

# Format: <upstream repo name>@
#         <upstream repo url>@
#         <upstream branch>@
#         <base revision on chosen upstream branch>@
#         <clone depth>
REVS_MAP=("DeepRecSys@https://github.com/harvard-acc@master@1383176@$NO_DEPTH"
          "dlrm@https://github.com/facebookresearch@main@4a9d0182@$NO_DEPTH"
          "pytorch@https://github.com/pytorch@release/2.0@c263bd43e8@$DEFAULT_DEPTH"
          "gramine@https://github.com/gramineproject@master@37550e1@$NO_DEPTH"
          "qemu@https://github.com/qemu@v7.2.0@b67b00e@$DEFAULT_DEPTH"
          "DRAMsim3@https://github.com/umd-memsys@master@2981759@$DEFAULT_DEPTH"
          "linux@https://git.kernel.org/pub/scm/linux/kernel/git/stable@v6.1.6@38f3ee126@$DEFAULT_DEPTH")

PNM_LIB_DIR="PNMLibrary"

rm -fr $UPSTREAM_DIR && mkdir -p $UPSTREAM_DIR

cd $RELEASE_DIR

setup_pnm_lib_repo() {
  (cd $PNM_LIB_DIR; patch -p1 < $PATCHES_DIR/$PNM_LIB_DIR/*.patch)
}

setup_upstream_repo() {
  URL="${2}/${1}"
  DEPTH_OPT="--depth $5"
  if [ "x$5" = "x$NO_DEPTH" ];
  then
    DEPTH_OPT=""
  fi
  (cd $UPSTREAM_DIR && git clone $URL -b $3 $DEPTH_OPT && cd $1 && git checkout $4 && \
   git checkout -b pnm && git am $PATCHES_DIR/$1/*.patch)
}

setup_pnm_lib_repo

for rec in "${REVS_MAP[@]}"; do
  REPO=$(echo "$rec" | awk -F "@" '{print $1}')
  UPSTREAM_PREFIX=$(echo "$rec" | awk -F "@" '{print $2}')
  BRANCH=$(echo "$rec" | awk -F "@" '{print $3}')
  REV=$(echo "$rec" | awk -F "@" '{print $4}')
  DEPTH=$(echo "$rec" | awk -F "@" '{print $5}')
  setup_upstream_repo $REPO $UPSTREAM_PREFIX $BRANCH $REV $DEPTH
done
