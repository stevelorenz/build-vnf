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

import ctypes as ct
import sys
import time

from bcc import BPF

import pyroute2

DEBUG = True


def usage():
    print("Usage: {0} <ingress ifdev> <egress ifdev>".format(sys.argv[0]))
    print("\te.g.: {0} eth0 eth1\n".format(sys.argv[0]))
    sys.exit(1)


if len(sys.argv) != 3:
    usage()
else:
    in_if_dev = sys.argv[1]
    eg_if_dev = sys.argv[2]

ip_route = pyroute2.IPRoute()
# Get index of the egress interface
eg_if_dev_idx = ip_route.link_lookup(ifname=eg_if_dev)[0]

bpf = BPF(src_file="./xdp_udp_forwarding.c", cflags=["-w", "-Wall"])
# Add egress interface index into the DEV map
tx_port = bpf.get_table("tx_port")
tx_port[0] = ct.c_int(eg_if_dev_idx)
# Attach XDP functions
in_fn = bpf.load_func("ingress_xdp_redirect", BPF.XDP)
eg_fn = bpf.load_func("egress_xdp_tx", BPF.XDP)
print("Attach XDP filter to ingress device: %s" % in_if_dev)
print("Attach XDP filter to egress device: %s" % eg_if_dev)
bpf.attach_xdp(in_if_dev, in_fn, 0)
bpf.attach_xdp(eg_if_dev, eg_fn, 0)

udp_nb = bpf["udp_nb"]
print("Start main loop, hit CTRL+C to stop")
while True:
    try:
        if DEBUG:
            nb = udp_nb.sum(0).value
            print("Number of received UDP segment: {}".format(nb))
            time.sleep(0.5)
    except KeyboardInterrupt:
        break
print("Remove XDP filter from device: %s" % in_if_dev)
bpf.remove_xdp(in_if_dev, flags=0)
print("Remove XDP filter from device: %s" % eg_if_dev)
bpf.remove_xdp(eg_if_dev, flags=0)
