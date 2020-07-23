#!/bin/bash
#
# Run DPDK L2 Power application INSIDE the virtual setup managed by ./benchmark-two-direct.py
#

if [[ ! -e ../../build/related_works/ffpp_l2fwd_power ]]; then
        echo "Can not find the directory of l2fwd_power sample."
        exit 1
fi

cd ../../build/related_works/

echo "* Run DPDK l2fwd_power on virtual interfaces."

# - Add --empty-poll to enable empty mode with the thresholds configured.
# --log-level user1,8 can be used for debugging.
if [[ $1 == "-t" ]]; then
        ./ffpp_l2fwd_power -l 0,1 \
                --vdev net_af_packet0,iface=vnf-in --vdev net_af_packet1,iface=vnf-out \
                --no-pci --single-file-segments --file-prefix=vnf --log-level=eal,3 \
                -- -p 0x03 --no-mac-updating --no-pm -T 0
else
        xdp-loader unload vnf-in
        xdp-loader unload vnf-out
        ./ffpp_l2fwd_power -l 0,1 \
                --vdev net_af_xdp0,iface=vnf-in net_af_xdp1,iface=vnf-out \
                --no-pci --single-file-segments --file-prefix=vnf \
                -- -p 0x03 --no-mac-updating -T 0
fi
