#!/bin/bash

echo "* Load ./xdp_pass_kern.o on interface lo with skb-mode."
./xdp_pass_user --dev lo --skb-mode
ip link show dev lo

echo "* Run 'ip link set dev lo xdpgeneric off' to remove the loaded XDP program."
