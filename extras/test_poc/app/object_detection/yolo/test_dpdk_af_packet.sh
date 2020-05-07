#! /bin/bash
# About: Check if DPDK PMD for af_packet works properly

cd "$RTE_SDK/build/app" || exit

EAL_PARAMS="-m 512 \
    --file-prefix testpmd \
    --no-pci \
    --single-file-segments \
    --vdev=eth_af_packet0,iface=eth0 \
    --log-level=eal,8
"

TESTPMD_PARAMS="--burst=1 rxq=1 --txq=1"

./testpmd $EAL_PARAMS -- $TESTPMD_PARAMS
