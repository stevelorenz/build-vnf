#!/bin/bash
# About: Start pktgen on a node with 2 lcores

BIN="${HOME}/bin/"
# Update config script
cp ./test_cfg.pkt ${BIN}/test_cfg.pkt
cd ${BIN}
sudo ./pktgen -l 0,1 -- -P -m 1.0 -f ./test_cfg.pkt
