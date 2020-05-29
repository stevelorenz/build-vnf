#!/bin/bash

if [[ ! -d /opt/dpdk/examples/l2fwd ]]; then
    echo "Can not find the directory of l2fwd sample."
    exit 1
fi

cd /opt/dpdk/examples/l2fwd/
if [[ !  -d /opt/dpdk/examples/l2fwd/build ]]; then
    make
fi

echo "* Run l2fwd with AF_XDP PMD on interface vnf-in."
cd /opt/dpdk/examples/l2fwd/build
./l2fwd -l 0 --vdev net_af_xdp,iface=vnf-in --no-pci --single-file-segments  -- -p 0x01
