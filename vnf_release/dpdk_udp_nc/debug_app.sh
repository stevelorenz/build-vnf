#!/bin/bash
# About: Debug udp_nc app with gdb
#        Also enable packet-capturing to dump packets into pcap file

SRC_MAC="08:00:27:3c:97:68"
DST_MAC="08:00:27:e1:f1:7d"
SOCKET_MEM=100
LCORES="0"
POLL_PAR="10,100,100"

if [[ "$#" -ne 1 ]]; then
    echo "Illegal number of parameters."
    echo "The type of coder MUST be given."
    echo "NC coder type:"
    echo "  0: Encoder"
    echo "  1: Decoder"
    echo "  2: Recoder"
    echo " -1: Simple forwarding"
    exit 1
fi

sudo gdb --args ./build/udp_nc -l $LCORES -m $SOCKET_MEM -- \
    -o "$1" -b 1 -t 10 \
    -i $POLL_PAR \
    -n 2 -p 0x3 -q 2\
    -s $SRC_MAC -d $DST_MAC \
    --packet-capturing --debugging
