#!/bin/bash
# About: Run udp_test app
#
#  MARK:
#        - For measurements, the socket memory should be increased to e.g. 1024MB
#        - For CLI options, check the l2fwd_usage() function in the main.c
#          Or run $sudo ./build/udp_test -l 0 -m 100 -- -h
#        - For measurements, -v 0 should be used to avoid debugging massages.

function help() {
    echo "\$run_app.sh proc_opt max_pkt_burst drain_tx_us"
    echo "-proc_opt: UDP segments processing operations"
    echo "  0: Forwarding"
    echo "  1: Appending timestamp"
    echo "  2: XOR a random key"
    echo "-max_pkt_burst: Maximal number of burst packets"
    echo "-drain_tx_us: Period to drain the tx queue"
}

if [[ "$#" -ne 3 ]]; then
    echo "Illegal number of parameters"
    help
    exit 1
fi

sudo ./build/udp_test -l 0 -m 100 -- \
    -p 0x3 -q 2 -s 08:00:27:3c:97:68 -d 08:00:27:e1:f1:7d \
    -o "$1" -b "$2" -t "$3" \
    -i 10,100,5000000 \
