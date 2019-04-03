#! /bin/bash
# About: Run tests for fast_io core functions

EAL_PARAMS="-l 0 -m 512 \
    --file-prefix testpmd \
    --no-pci \
    --single-file-segments \
    --vdev=eth_af_packet0,iface=eth0 \
    --log-level=eal,8"

if [[ $1 == "-d" ]]; then
    gdb --args ./test_l2_fwd.out $EAL_PARAMS
    #lldb -- ./test_l2_fwd.out $EAL_PARAMS
else
    ./test_l2_fwd.out $EAL_PARAMS
fi
