#!/bin/bash
TEST_DIR="scripts/pnm/zswap/tests"
MADVISE_DIR="${TEST_DIR}/madvise"
SCRIPT_DIR="scripts/pnm/zswap/qemu"

MODULE_PARAM_DIR="/sys/module/pnm_zswap_sim/parameters"
MODULE_PARAM_COMPRESSOR="${MODULE_PARAM_DIR}/compressor"
MODULE_PARAM_ENABLE="${MODULE_PARAM_DIR}/enabled"

function prepare_zswap() {
    # Build
    cd ${MADVISE_DIR}; make; cd -
    cd ${TEST_DIR}/memset; make; cd -
    # Swap initialization
    sudo ${SCRIPT_DIR}/swap_setting.sh
    # zSwap initialization in FS mode with 'deflate' compression algorithm
    sudo ${SCRIPT_DIR}/pnm_sim_mode.sh
}

function enable_simulator() {
    echo "Y" | sudo tee $MODULE_PARAM_ENABLE
}

function disable_simulator() {
    echo "N" | sudo tee $MODULE_PARAM_ENABLE
}

# Possible modes: deflate, lzo, ps
function switch_mode() {
    disable_simulator
    echo "$1" | sudo tee $MODULE_PARAM_COMPRESSOR
    enable_simulator
}
