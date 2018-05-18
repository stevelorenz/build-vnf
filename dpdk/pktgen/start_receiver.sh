#!/bin/bash
# About: Start pktgen as a receiver on a node with 2 lcores

BIN="${HOME}/bin/"
CONFIG="receiver_cfg.lua"
#CONFIG="test_seq.lua"
#CONFIG="receiver_cfg.pkt"
LOG="udp_recv.log"

# Update config script
cp ./${CONFIG} ${BIN}/${CONFIG}
cd ${BIN}
sudo ./pktgen -l 0,1 -- -P -m 1.0 -f ./${CONFIG} -l ./${LOG}
