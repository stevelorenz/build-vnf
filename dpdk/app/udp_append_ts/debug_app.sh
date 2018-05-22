#!/bin/bash
# About: An example of running the forwarding APP
# Param: Use the first lcore for both rx and tx ports

sudo gdb --args ./build/udp_append_ts -l 0 -- -p 0x3 -q 2 -d 9c:da:3e:6f:ba:df --packet-capturing --debugging
