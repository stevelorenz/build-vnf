#!/bin/bash
# About: An example of running the forwarding APP
# Param: Use the first lcore for both rx and tx ports

sudo gdb --args ./build/udp_append_ts -l 0 -m 100 -- -p 0x3 -q 2 -s 9c:da:3e:6f:ba:00 -d 9c:da:3e:6f:ba:11 -v 1 --packet-capturing --debugging
