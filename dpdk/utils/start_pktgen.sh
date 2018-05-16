#!/bin/bash
# About: Start pktgen on a node with 2 lcores

BIN="${HOME}/bin/"
cd ${BIN}
sudo ./pktgen -l 0,1 -- -P -m 1.0
