#!/bin/bash
#
# Run DPDK L2 Power application.
#

if [[ ! -e ../../build/related_works/ffpp_l2fwd_power ]]; then
    echo "Can not find the directory of l2fwd_power sample."
    exit 1
fi

cd ../../build/related_works/ || exit

echo "* Run DPDK l2fwd_power on virtual interfaces."

# - Add --empty-poll to enable empty mode with the thresholds configured.
# --log-level user1,8 can be used for debugging.
# -m: The power management mode:
#       - 0: No power management.
#       - 1: Code-instrumentation legacy approach.
#       - 2:
# MARK: mac-updating is required for the veth-based setup. Otherwise, the TX
# path does not work and the Trex cannot get packets back.
if [[ $1 == "-t" ]]; then
    ./ffpp_l2fwd_power -l 0,1 \
        --vdev net_af_packet0,iface=vnf-in --vdev net_af_packet1,iface=vnf-out \
        --no-pci --single-file-segments --file-prefix=vnf --log-level=eal,3 \
        -- -p 0x03 -m 0 -T 0
elif [[ $1 == "-tm" ]]; then # Test Malte (to screen vnf-in)
    ./ffpp_l2fwd_power -l 1,3 \
        --vdev net_af_packet0,iface=vnf-in --vdev net_af_packet1,iface=vnf-out \
        --no-pci --single-file-segments --file-prefix=vnf --log-level=eal,3 \
        -- -p 0x03 -m 0 -T 0 <<<'y'
elif [[ $1 == "-tc" ]]; then
    ./ffpp_l2fwd_power -l 0,1 \
        --vdev net_af_packet0,iface=vnf-in --vdev net_af_packet1,iface=vnf-out \
        --no-pci --single-file-segments --file-prefix=vnf --log-level=eal,3 \
        -- -p 0x03 -m 0 -T 0 --enable-crypto 2
elif [[ $1 == "-c" ]]; then # Test crypto
    REPS="5"
    if [ "$2" ]; then
        REPS="$2"
    fi
    xdp-loader unload vnf-in
    xdp-loader unload vnf-out
    ./ffpp_l2fwd_power -l 1,3 \
        --vdev net_af_xdp0,iface=vnf-in --vdev net_af_xdp1,iface=vnf-out \
        --no-pci --single-file-segments --file-prefix=vnf --log-level=eal,3 \
        -- -p 0x03 -m 0 -T 0 --enable-crypto $REPS <<<'y'
elif [[ $1 == "-m" ]]; then # Bare vnf0 w/o code instrumentation PM
    xdp-loader unload vnf-in
    xdp-loader unload vnf-out
    ./ffpp_l2fwd_power -l 1,3 \
        --vdev net_af_xdp0,iface=vnf-in --vdev net_af_xdp1,iface=vnf-out \
        --no-pci --single-file-segments --file-prefix=vnf0 --log-level=eal,3 \
        -- -p 0x03 -m 0 -T 0 <<<'y'
elif [[ $1 == "-m1" ]]; then # Bare vnf0 w/o code instrumentation PM
    xdp-loader unload vnf-in
    xdp-loader unload vnf-out
    ./ffpp_l2fwd_power -l 5,7 \
        --vdev net_af_xdp0,iface=vnf-in --vdev net_af_xdp1,iface=vnf-out \
        --no-pci --single-file-segments --file-prefix=vnf1 --log-level=eal,3 \
        -- -p 0x03 -m 0 -T 0 <<<'y'
elif [[ $1 == "-p" ]]; then # vnf with code instrumentation PM
    xdp-loader unload vnf-in
    xdp-loader unload vnf-out
    ./ffpp_l2fwd_power -l 1,3,5,7 \
        --vdev net_af_xdp0,iface=vnf-in --vdev net_af_xdp1,iface=vnf-out \
        --no-pci --single-file-segments --file-prefix=vnf --log-level=eal,3 \
        -- -p 0x03 -m 1 -T 0 <<<'y'
elif [[ $1 == "-mp" ]]; then # Setup to measure custom PM
    xdp-loader unload vnf-in
    xdp-loader unload vnf-out
    ./ffpp_l2fwd_power -l 3,5 \
        --vdev net_af_xdp0,iface=vnf-in --vdev net_af_xdp1,iface=vnf-out \
        --no-pci --single-file-segments --file-prefix=vnf --log-level=eal,3 \
        -- -p 0x03 -m 0 -T 0 <<<'y'
else
    xdp-loader unload vnf-in
    xdp-loader unload vnf-out
    ./ffpp_l2fwd_power -l 0,1 \
        --vdev net_af_xdp0,iface=vnf-in net_af_xdp1,iface=vnf-out \
        --no-pci --single-file-segments --file-prefix=vnf \
        -- -p 0x03 -m 1 -T 0
fi
