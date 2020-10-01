#!/bin/bash

if [ "$EUID" -ne 0 ]; then
    echo "Please run this script with sudo."
    exit
fi

set -o errexit

./benchmark-local.py --setup_name two_veth_xdp_fwd --pktgen_image trex:v2.81 teardown
./benchmark-local.py --setup_name two_veth_xdp_fwd --pktgen_image trex:v2.81 setup
./benchmark-local.py --setup_name two_veth_xdp_fwd --pktgen_image trex:v2.81 teardown
