#!/bin/bash
# About: Start pktgen on a node with 2 lcores

BIN="${HOME}/bin/"
CONFIG="sender_cfg.pkt"

# Update config script
cp ./${CONFIG} ${BIN}/${CONFIG}
cd ${BIN}
sudo ./pktgen -l 0,1 -- -P -m 1.0 -f ./${CONFIG}
