#! /bin/bash
#
# init_ifce.sh
# About: Init ingress and egress interfaces
#

in_ifce=$1
out_ifce=$2
init_opt=$3

if [[ "$init_opt" = '-offloading-off' ]]; then
    echo "Disable offloading via ethtool."
    sudo ethtool --offload "$in_ifce" rx off tx off
    sudo ethtool --offload "$out_ifce" rx off tx off
fi
