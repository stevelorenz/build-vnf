#!/bin/bash
# About: Debug udp_append_ts app

sudo gdb --args ./build/udp_append_ts -l 0 -m 100 -- -p 0x3 -q 2 -s -s fa:16:3e:d1:72:c2 -d fa:16:3e:8b:b3:5f -v 1 --packet-capturing --debugging
