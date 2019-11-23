#!/bin/bash

./build/l2fwd -l 0,1 --master-lcore 0 -m 256 \
        --vdev=eth_af_packet0,iface=relay-s1 --no-pci --single-file-segments \
        --log-level=eal,1 --log-level=user2,8 --log-level=user3,8 \
        -- -p 1 -e "1,0,0" --no-mac-updating
