#!/bin/bash
# About: Debug udp_test app with gdb
#        Also enable packet-capturing to dump packets into pcap file

sudo gdb --args ./build/udp_test -l 0 -m 100 -- \
    -p 0x3 -q 2 -s -s fa:16:3e:d1:72:c2 -d fa:16:3e:8b:b3:5f \
    -i 10,1000,5000000 \
    -v 1 --packet-capturing --debugging
