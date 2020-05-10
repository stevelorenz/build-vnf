#!/bin/bash
# About: Load ./xdp_drop_kern.o on a network interface using xdp-loader provided by xdp-tools.

if [ "$#" -ne 1 ]; then
    echo "Illegal number of parameters"
    echo "Usage: ./load_xdp.h interface_name. E.g. lo or eth0."
    exit 1
fi

echo "* Load ./xdp_drop_kern.o on interface lo with skb-mode."
xdp-loader load --mode skb $1 ./xdp_drop_kern.o
ip link show dev $1

echo "* Run 'xdp-loader unload $1' to remove the loaded XDP program."
