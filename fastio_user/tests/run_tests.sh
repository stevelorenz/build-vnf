#! /bin/bash
# About:

EAL_PARAMS="-m 512 \
    --file-prefix testpmd \
    --no-pci \
    --single-file-segments \
    --vdev=eth_af_packet0,iface=eth0 \
    --log-level=eal,8"

./test.out $EAL_PARAMS
