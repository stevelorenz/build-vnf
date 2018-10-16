#! /bin/bash
#
# start_pdump.sh
# About: Start dpdk-pdump tool to capture packets and stored in a pcap file
#        Capture all tx packets of the egress interface

# Path to the DPDK dir
export RTE_SDK=${HOME}/dpdk

sudo "${RTE_SDK}/build/app/dpdk-pdump" -- \
    --pdump 'port=1,queue=*,tx-dev=./p1_tx.pcap'
