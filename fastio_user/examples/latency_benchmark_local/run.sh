#!/usr/bin/env bash
#
# About: Run latency_benchmark_local example
#        Use root privileged user to run this script
#

# Fail on error
set -e

# Fail on unset var usage
set -o nounset

QUITE=false

# Default EAL parameters
EAL_CORE="-l 0"
HUGEPAGE_NUM_2048=256
EAL_MEM="-m $HUGEPAGE_NUM_2048"
EAL_IFACE="--vdev=eth_af_packet0,iface=eth0"
# EAL log level, use 8 for debugging, 1 for info
EAL_LOG_LEVEL=1
EAL_MISC="--no-pci --single-file-segments --log-level=eal,$EAL_LOG_LEVEL"

function latency_benchmark_local() {
    # MARK: Use one master core and one slave core
    local EAL_CORE="-l 0,1"
    local EAL_PARAMS="$EAL_CORE $EAL_MEM $EAL_IFACE $EAL_MISC"
    local LOG="./latency_benchmark_local.log"

    if [[ $QUITE == true ]]; then
        ./latency_benchmark_local.out $EAL_PARAMS > "$LOG" 2>&1
    else
        time ./latency_benchmark_local.out $EAL_PARAMS
    fi
}

echo "*** Run latency_benchmark_local example..."
echo "--------------------------------------------------------------------------"

declare -r opt="${1-"default"}"
if [[ $opt == "-q" ]]; then
    echo "* Run in quite mode"
    QUITE=true
fi

python3 ./uds_server.py > /dev/null 2>&1 &
sleep 3
latency_benchmark_local
sleep 1
pgrep -u root 'python3' | xargs kill

echo "--------------------------------------------------------------------------"
