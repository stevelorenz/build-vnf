#!/bin/bash

if [[ ! -e ../../build/related_works/ffpp_power_hw_ctr ]]; then
        echo "Can not find the directory of ffpp_power_hw_ctr sample."
        exit 1
fi

cd ../../build/related_works/ || exit

echo "* Run DPDK ffpp_power_hw_ctr."

./ffpp_power_hw_ctr --proc-type=secondary \
        -l 0,1 \
        --no-pci --single-file-segments --file-prefix=vnf --log-level=eal,3 \
        --log-level=user1,8 \
        -- -b 0.01
