#! /bin/bash
# About: Run load_gen_image example

EAL_PARAMS="-l 0 -m 64 \
    --file-prefix load_gen_image\
    --no-pci \
    --single-file-segments \
    --vdev=eth_af_packet0,iface=eth0 \
    --log-level=eal,8"

./load_gen_image.out $EAL_PARAMS
