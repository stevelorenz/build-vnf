#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Forwarding UDP segments with XDP
Email: xianglinks@gmail.com

MARK : - XDP_TX (Kernel version 4.8) can only bounce packets to the
         SAME interface it arrived at.

Ref  : - bcc/example/networking/xdp
           - xdp_drop_count.py
           - xdp_redirect_map.py
       - http://docs.cilium.io/en/latest/bpf/#xdp
"""

import sys
import time

from bcc import BPF

import pyroute2


def usage():
    print("Usage: {0} <ifdev>".format(sys.argv[0]))
    print("\te.g.: {0} eth0\n".format(sys.argv[0]))
    sys.exit(1)


if len(sys.argv) != 2:
    usage()
else:
    if_dev = sys.argv[1]

bpf = BPF(src_file="./xdp_udp_forwarding.c", cflags=["-w"])

fn = bpf.load_func("xdp_udp_forwarding", BPF.XDP)
print("Attach XDP filter to device: %s" % if_dev)
bpf.attach_xdp(if_dev, fn, 0)

udp_nb = bpf["udp_nb"]
print("Start main loop, hit CTRL+C to stop")
while True:
    try:
        nb = udp_nb.sum(0).value
        print("Number of received UDP segment: {}".format(nb))
        time.sleep(0.5)
    except KeyboardInterrupt:
        break
print("Remove XDP filter from device: %s" % if_dev)
bpf.remove_xdp(if_dev, flags=0)
