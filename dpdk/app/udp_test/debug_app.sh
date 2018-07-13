#!/bin/bash
# About: Debug udp_test app with gdb
#        Also enable packet-capturing to dump packets into pcap file

if [[ "$#" -ne 3 ]]; then
    echo "Illegal number of parameters"
    exit 1
fi

sudo gdb --args ./build/udp_test -l 0 -m 100 -- \
    -q 1 -s -s fa:16:3e:d1:72:c2 -d fa:16:3e:8b:b3:5f \
    -o "$1" -b "$2" -t "$3" \
    -i 10,1000,5000000 \
    -n 1 -p 0x1 \
    --packet-capturing --debugging
