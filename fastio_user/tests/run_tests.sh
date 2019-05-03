#! /bin/bash
#
# About: Run tests for core functions of fastio_usr
#

# EAL log level, use 8 for debugging
EAL_LOG_LEVEL=1
IFACE=eth0
HUGE_MEM=256
LCORES=0

function run_l2_fwd_test() {
    echo ""
    echo "*** Run L2 FWD test..."
    EAL_PARAMS="-l $LCORES -m $HUGE_MEM \
        --file-prefix l2fwd \
        --no-pci \
        --single-file-segments \
        --vdev=eth_af_packet0,iface=$IFACE \
        --log-level=eal,$EAL_LOG_LEVEL"

    ./test_l2_fwd.out $EAL_PARAMS
}

function run_mp_test() {
    echo ""
    echo "*** Run multi-process test..."
    EAL_PARAMS_MP_PRI="-l $LCORES -m $HUGE_MEM \
        --file-prefix mp_primary \
        --no-pci \
        --single-file-segments \
        --vdev=eth_af_packet0,iface=$IFACE \
        --log-level=eal,$EAL_LOG_LEVEL --proc-type=primary"

    ./test_mp.out $EAL_PARAMS_MP_PRI
}

echo "# Run tests for core functions of fastio_user ..."
echo "--------------------------------------------------------------------------"
run_l2_fwd_test
run_mp_test
echo "--------------------------------------------------------------------------"
