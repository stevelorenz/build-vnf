#!/bin/bash
#
# Run DPDK L2 Power application INSIDE the virtual setup managed by ./benchmark-two-direct.py
#

if [[ ! -e ../../build/related_works/ffpp_l2fwd_power ]]; then
        echo "Can not find the directory of l2fwd_power sample."
        exit 1
fi

cd ../../build/related_works/

echo "* Run DPDK l2fwd_power on interface vnf-in"
echo "WARN: ONLY one interface is used since common laptops have two physical cores."
if [[ $1 == "-t" ]]; then
        ep_args=""
        if [[ $2 == "-t" ]]; then
                ep_args="1,0,0"
        fi
        ./ffpp_l2fwd_power -l 0,1 --vdev net_af_packet0,iface=vnf-in \
                --no-pci --single-file-segments --file-prefix=vnf --log-level=eal,3 \
                -- -p 0x01 --no-mac-updating --empty-poll="$ep_args"
else
        xdp-loader unload vnf-in
        xdp-loader unload vnf-out
        ./ffpp_l2fwd_power -l 0,1 --vdev net_af_xdp0,iface=vnf-in \
                --no-pci --single-file-segments --file-prefix=vnf \
                -- -p 0x01 --no-mac-updating --empty-poll="$ep_args"
fi
