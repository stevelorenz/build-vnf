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

echo "$TEST"

cd /opt/dpdk/examples/l2fwd/build

if [ "$TEST" = true ]; then
    echo "* Run DPDK l2fwd with af_packet on interfaces eno2 and eno3."
./l2fwd -l 0,1 --vdev net_af_packet0,iface=eno2 --vdev net_af_packet1,iface=eno3 --no-pci --single-file-segments --file-prefix=vnf  -- -p 0x03
else
    echo "* Run DPDK l2fwd with af_xdp on interfaces eno2 and eno3."
./l2fwd -l 0,1 --vdev net_af_xdp0,iface=eno2 --vdev net_af_xdp1,iface=eno3 --no-pci --single-file-segments --file-prefix=vnf  -- -p 0x03
fi
