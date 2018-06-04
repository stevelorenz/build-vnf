#!/bin/bash
# About: Run udp_append_ts app
#  MARK: For measurements, the socket memory should be increased to e.g. 1024MB
#        For CLI options, check the l2fwd_usage() function in the main.c
#        Or run $sudo ./build/udp_append_ts -l 0 -m 100 -- -h


sudo ./build/udp_append_ts -l 0 -m 100 -- \
	-p 0x3 -q 2 -s fa:16:3e:d1:72:c2 -d fa:16:3e:8b:b3:5f \
	-i 10,100,5000000 \
	-v 1
