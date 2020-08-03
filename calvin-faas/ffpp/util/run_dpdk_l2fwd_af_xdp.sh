#!/bin/bash
#
# Run DPDK L2 forwarding sample application INSIDE the virtual setup managed by ./benchmark-two-direct.py
#

if [[ ! -d /opt/dpdk/examples/l2fwd ]]; then
    echo "Can not find the directory of l2fwd sample."
    exit 1
fi

cd /opt/dpdk/examples/l2fwd/
if [[ ! -d /opt/dpdk/examples/l2fwd/build ]]; then
    make
fi

echo "* Run DPDK l2fwd on interfaces vnf-in and vnf-out."
cd /opt/dpdk/examples/l2fwd/build

if [[ $1 == "-t" ]]; then
    ./l2fwd -l 0,1 --vdev net_af_packet0,iface=vnf-in --vdev net_af_packet1,iface=vnf-out --no-pci --single-file-segments --file-prefix=vnf -- -p 0x03
else
    xdp-loader unload vnf-in
    xdp-loader unload vnf-out
    ulimit -l unlimited
    ./l2fwd -l 0,1 --vdev net_af_xdp0,iface=vnf-in --vdev net_af_xdp1,iface=vnf-out --no-pci --single-file-segments --file-prefix=vnf -- -p 0x03
fi
