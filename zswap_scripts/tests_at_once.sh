#!/bin/bash
CI_SCRIPT_DIR=$(dirname ${0})
source ${CI_SCRIPT_DIR}/common_defs.sh
source ${CI_SCRIPT_DIR}/common.sh

# Tests
function madvise_test() {
    cd $MADVISE_DIR
    ./multiple_instances.sh
    ./multiple_threads.sh
    cd -
}

function gather_stats() {
    sudo grep -R . /sys/kernel/debug/pnm_zswap
}

function run_tests_in_mode() {
    switch_mode "${1}"
    madvise_test
    gather_stats "${1}"
}

prepare_zswap

# FS mode 'deflate' compression algorithm
run_tests_in_mode "deflate"

# FS mode 'lzo' compression algorithm
run_tests_in_mode "lzo"

# PS mode
run_tests_in_mode "ps"

# ssh will hang on Qemu if simulation mode is not disabled
disable_simulator
