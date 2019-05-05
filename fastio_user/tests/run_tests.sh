#! /bin/bash
#
# About: Run tests for core functions of fastio_usr
#

# EAL log level, use 8 for debugging, 1 for info
EAL_LOG_LEVEL=8
IFACE=eth0
HUGE_MEM=256

function run_l2_fwd_test() {
    echo ""
    echo "*** Run L2 FWD test..."
    EAL_PARAMS="-l 0 -m $HUGE_MEM \
        --file-prefix l2fwd \
        --no-pci \
        --single-file-segments \
        --vdev=eth_af_packet0,iface=$IFACE \
        --log-level=eal,$EAL_LOG_LEVEL"

    LOG="./test_l2_fwd.log"
    ./test_l2_fwd.out $EAL_PARAMS > $LOG
}

function run_mp_test() {
    echo ""
    echo "*** Run multi-process test..."
    EAL_PARAMS_MP_PRI="-l 0 -m $HUGE_MEM \
        --file-prefix mp_primary \
        --no-pci \
        --single-file-segments \
        --vdev=eth_af_packet0,iface=$IFACE \
        --log-level=eal,$EAL_LOG_LEVEL"

    EAL_PARAMS_MP_SECOND="-l 1 -m $HUGE_MEM \
        --file-prefix mp_primary \
        --no-pci \
        --single-file-segments \
        --vdev=eth_af_packet0,iface=$IFACE \
        --log-level=eal,$EAL_LOG_LEVEL"

    LOG_PRIM="./test_mp_prim.log"
    LOG_SECOND="./test_mp_second.log"
    ./test_mp.out $EAL_PARAMS_MP_PRI --proc-type=primary > $LOG_PRIM &
    sleep 2
    ./test_mp.out $EAL_PARAMS_MP_SECOND --proc-type=secondary > $LOG_SECOND
}

echo "# Run tests for core functions of fastio_user ..."
echo "--------------------------------------------------------------------------"
rm ./*.log > /dev/null 2>&1
run_l2_fwd_test 2>&1 | grep error
#run_mp_tes
echo "--------------------------------------------------------------------------"
