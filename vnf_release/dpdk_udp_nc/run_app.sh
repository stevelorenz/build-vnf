#!/bin/bash
# About: Run udp_nc DPDK app
#

SRC_MAC="08:00:27:3c:97:68"
DST_MAC="08:00:27:e1:f1:7d"
SOCKET_MEM=100
LCORES="0"
POLL_PAR="10,100,100"

function help() {
    echo "Usage: \$run_app.sh coder_type max_pkt_burst drain_tx_us nb_ports port_mask nb_queue"
    echo "-coder_type: NC coder type"
    echo "  0: Encoder"
    echo "  1: Decoder"
    echo "  2: Recoder"
    echo " -1: Simple forwarding"
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
    --kni-mode
