#!/bin/bash
# About: Debug udp_nc app with gdb
#        Also enable packet-capturing to dump packets into pcap file

if [[ "$#" -ne 1 ]]; then
    echo "Illegal number of parameters."
    echo "The type of coder must be given."
    exit 1
fi

sudo gdb --args ./build/udp_nc -l 0 -m 100 -- \
    -q 2 -s -s 08:00:27:3c:97:68 -d 08:00:27:e1:f1:7d \
    -o "$1" -b 1 -t 1 \
    -i 10,1000,1000000 \
    -n 2 -p 0x3 \
    --packet-capturing --debugging
