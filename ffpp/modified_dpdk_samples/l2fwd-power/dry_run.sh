#!/bin/bash

./build/l2fwd -l 1 -m 256 --vdev=eth_af_packet0,iface=relay-s1 --no-pci --single-file-segments -- -p 1 --no-mac-updating --no-power-management
