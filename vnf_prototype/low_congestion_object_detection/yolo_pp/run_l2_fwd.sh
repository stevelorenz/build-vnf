#! /bin/bash
# About:

if [[ $1 == "-d" ]]; then
    EAL_PARAMS="-l 0 -m 512 \
        --file-prefix testpmd \
        --no-pci \
        --single-file-segments \
        --vdev=eth_af_packet0,iface=eth0 \
        --log-level=eal,8"

    gdb --args ./l2_fwd.out $EAL_PARAMS
else
    EAL_PARAMS="-l 0 -m 512 \
        --file-prefix testpmd \
        --no-pci \
        --single-file-segments \
        --vdev=eth_af_packet0,iface=eth0 \
        --log-level=eal,8"

    ./l2_fwd.out $EAL_PARAMS

fi


