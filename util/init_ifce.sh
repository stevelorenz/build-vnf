#! /bin/bash
#
# init_ifce.sh
# About: Init ingress and egress interfaces
#

in_ifce=$1
out_ifce=$2

# Disable checksum offloading
sudo ethtool --offload $in_ifce rx off tx off
sudo ethtool --offload $out_ifce rx off tx off
