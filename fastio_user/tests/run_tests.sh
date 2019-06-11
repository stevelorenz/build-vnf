#!/usr/bin/env bash
#
# About: Run tests for core functions of FastIO-User library
#

# Fail on error
set -e

# Fail on unset var usage
set -o nounset

TEST_FUNS=(test_l2_fwd test_ring)

QUITE=false

# Default EAL parameters
EAL_CORE="-l 0"
HUGEPAGE_NUM_2048=256
EAL_MEM="-m $HUGEPAGE_NUM_2048"
EAL_IFACE="--vdev=eth_af_packet0,iface=eth0"
# EAL log level, use 8 for debugging, 1 for info
EAL_LOG_LEVEL=8
EAL_MISC="--no-pci --single-file-segments --log-level=eal,$EAL_LOG_LEVEL"

function test_l2_fwd() {
    local EAL_PARAMS="$EAL_CORE $EAL_MEM $EAL_IFACE $EAL_MISC"
    local LOG="./test_l2_fwd.log"

    echo "- Run L2 forwarding test..."
    if [[ $QUITE == true ]]; then
        ./test_l2_fwd.out $EAL_PARAMS > "$LOG" 2>&1
    else
        ./test_l2_fwd.out $EAL_PARAMS
    fi
}

function test_ring() {
    # MARK: Use one master core and one slave core
    local EAL_CORE="-l 0,1"
    local EAL_PARAMS="$EAL_CORE $EAL_MEM $EAL_IFACE $EAL_MISC"
    local LOG="./test_ring.log"

    echo "- Run ring test..."
    if [[ $QUITE == true ]]; then
        ./test_ring.out $EAL_PARAMS > "$LOG" 2>&1
    else
        ./test_ring.out $EAL_PARAMS
    fi
}

echo "*** Run tests for core functions of fastio_user library"
echo "--------------------------------------------------------------------------"

declare -r opt="${1-"default"}"
if [[ $opt == "-q" ]]; then
    echo "* Run in quite mode"
    QUITE=true
fi

for func in "${TEST_FUNS[@]}"
do
    $func
done

echo "--------------------------------------------------------------------------"
