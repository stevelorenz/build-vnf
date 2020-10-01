#!/bin/bash
# About: Run per-packet latency evaluation for advanced functions

SOCKET_MEM=100
LCORES="0"

for ((i = 0; i < 1; i++)); do
    sudo ./build/eval -l $LCORES -m $SOCKET_MEM
done
