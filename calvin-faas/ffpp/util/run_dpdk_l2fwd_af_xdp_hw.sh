#!/bin/bash

if [[ ! -d /opt/dpdk/examples/l2fwd ]]; then
    echo "Can not find the directory of l2fwd sample."
    exit 1
fi

cd /opt/dpdk/examples/l2fwd/
if [[ ! -d /opt/dpdk/examples/l2fwd/build ]]; then
    make
fi

TEST=false

# Set poll mode driver
while :; do
    case $1 in
    -t)
        TEST=true
        ;;
    *)
        break
        ;;
    esac
    shift
done

cd /opt/dpdk/examples/l2fwd/build

if [ "$TEST" = true ]; then
    echo "* Run DPDK l2fwd with af_packet on interfaces eno2 and eno3."
    # ./l2fwd -l 1,3 --vdev net_af_packet0,iface=enp5s0f0 --vdev net_af_packet1,iface=enp5s0f1 --no-pci --single-file-segments --file-prefix=vnf  -- -p 0x03 --no-mac-updating
    ./l2fwd -l 1,3 --vdev net_af_packet0,iface=vnf-in --vdev net_af_packet1,iface=vnf-out --no-pci --single-file-segments --file-prefix=vnf -- -p 0x03 --no-mac-updating
else
    echo "* Run DPDK l2fwd with af_xdp on interfaces eno2 and eno3."
    xdp-loader unload vnf-in
    xdp-loader unload vnf-out
    ./l2fwd -l 1,3 --vdev net_af_xdp0,iface=vnf-in --vdev net_af_xdp1,iface=vnf-out --no-pci --single-file-segments --file-prefix=vnf -- -p 0x03 --no-mac-updating -T 0
    # ./l2fwd -l 1,3 --vdev net_af_xdp0,iface=enp5s0f0 --vdev net_af_xdp1,iface=enp5s0f1 --no-pci --single-file-segments --file-prefix=vnf  -- -p 0x03 --no-mac-updating
fi
