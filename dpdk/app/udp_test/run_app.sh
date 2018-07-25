#!/bin/bash
# About: Run udp_test app
#

SRC_MAC="08:00:27:3c:97:68"
DST_MAC="08:00:27:e1:f1:7d"
SOCKET_MEM=100
LCORES="0"
POLL_PAR="10,100,100"

function help() {
    echo "Usage: \$run_app.sh proc_opt max_pkt_burst drain_tx_us nb_ports port_mask"
    echo "-proc_opt: UDP segments processing operations"
    echo "  0: Forwarding"
    echo "  1: Appending timestamp"
    echo "  2: XOR a random key"
    echo "-max_pkt_burst: Maximal number of burst packets"
    echo "-drain_tx_us: Period to drain the tx queue"
    echo "-nb_ports: Number of to be used ports"
    echo "-port_mask: Port mask of to be used ports, in hex"
    echo "-nb_queue: Number of receive queues"
}

if [[ "$#" -ne 6 ]]; then
    echo "Illegal number of parameters"
    help
    exit 1
fi

sudo ./build/udp_nc -l $LCORES -m $SOCKET_MEM -- \
    -s $SRC_MAC -d $DST_MAC \
    -i $POLL_PAR \
    -o "$1" -b "$2" -t "$3" \
    -n "$4" -p "$5" -q "$6" \
