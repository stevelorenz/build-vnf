#!/bin/bash
# About: Run udp_nc DPDK app
#

function help() {
    echo "Usage: \$run_app.sh coder_type max_pkt_burst drain_tx_us nb_ports port_mask"
    echo "-coder_type: NC coder type"
    echo "  0: Encoder"
    echo "  1: Decoder"
    echo "  2: Recoder"
    echo "-max_pkt_burst: Maximal number of burst packets"
    echo "-drain_tx_us: Period to drain the tx queue"
    echo "-nb_ports: Number of to be used ports"
    echo "-port_mask: Port mask of to be used ports, in hex"
}

if [[ "$#" -ne 5 ]]; then
    echo "Illegal number of parameters"
    help
    exit 1
fi

sudo ./build/udp_nc -l 0 -m 100 -- \
    -q 2 -s 08:00:27:3c:97:68 -d 08:00:27:e1:f1:7d \
    -o "$1" -b "$2" -t "$3" \
    -i 10,100,1000000 \
    -n "$4" \
    -p "$5" \
