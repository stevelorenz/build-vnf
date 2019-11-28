#!/bin/bash

echo "[WARN] For test/debugging. Do not use this for any perf test."
./build/l2fwd-power -l 1 --master-lcore 1 -m 256 \
        --vdev=eth_af_packet0,iface=relay-s1 --no-pci --single-file-segments \
        --log-level=eal,1 --log-level=user2,8 --log-level=power,8 \
        -- -p 1 -e "1,0,0" --no-mac-updating --enable-c-state
