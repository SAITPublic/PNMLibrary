#!/bin/bash
CI_SCRIPT_DIR=$(dirname ${0})
ARTIFACTS_DIR=${1}
PAGES_CNT=100000
THREADS_CNT=8
source ${CI_SCRIPT_DIR}/../common.sh
source ${CI_SCRIPT_DIR}/common.sh

ZSWAP_BENCH_DATA_PATH="/proc/pnm/zswap_bench"

prepare_zswap

# Refresh benchmark data
sudo echo 0 > $ZSWAP_BENCH_DATA_PATH

# Run memory_madvise
./${MADVISE_DIR}/memory_memadvise $PAGES_CNT $THREADS_CNT

# Collect benchmark data
RES_FILE="$ARTIFACTS_DIR/benchmark.txt"
echo "ZSWAP BENCHMARK ($PAGES_CNT PAGES, $THREADS_CNT THREADS):" > $RES_FILE
cat $ZSWAP_BENCH_DATA_PATH >> $RES_FILE

# ssh will hang on Qemu if simulation mode is not disabled
disable_simulator
