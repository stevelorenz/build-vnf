#!/bin/bash

echo "[WARN] For test/debugging. Do not use this for any perf test."

if [[ $1 == "distributor" ]]; then
    echo "- Run distributor as the primary process."
    ./distributor.out --proc-type=primary -l 1 --master-lcore 1 -m 128 \
        --vdev=eth_af_packet0,iface=h2-eth0 \
        --no-pci --single-file-segments --log-level=eal,8
elif [[ $1 == "dummy_vnf" ]]; then
    echo "- Run dummy_vnf as the secondary process."
    ./dummy_vnf.out --proc-type=secondary -l 1 --master-lcore 1 -m 128 \
        --no-pci --single-file-segments --log-level=eal,8
else
    echo "Unknown network function. Use distributor or dummy_vnf as the first parameter."
    exit 1
fi
