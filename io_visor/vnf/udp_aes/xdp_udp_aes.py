#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Encrypt UDP segments with AES

Email: xianglinks@gmail.com

Ref  : - bcc/example/networking/xdp
           - xdp_drop_count.py
           - xdp_redirect_map.py
       - http://docs.cilium.io/en/latest/bpf/#xdp
"""

import argparse
import ctypes as ct
import sys
import time

from bcc import BPF

import pyroute2

DEBUG = False

# MARK: Custom headers are currenly added into the CFLAGS. The BCC provides this
# currenly as a method to compile customized headers with llvm.

CFLAGS = ["-w", "-Wall",
          "-include", "../../../shared_lib/aes.h"]

parser = argparse.ArgumentParser(description='Encrypt UDP segments with AES')
parser.add_argument('-i', metavar='IFACE', type=str,
                    help='Ingress interface name. e.g. eth1')
parser.add_argument('-e', metavar='IFACE', type=str,
                    help='Egress interface name.')
parser.add_argument('--mode', type=str, default='CTR',
                    help='Block cipher mode of operation, default is CTR')
parser.add_argument('--debug', action='store_true',
                    help='Enable debugging mode.')
args = parser.parse_args()

in_if_dev = args.i
eg_if_dev = args.e

if args.debug:
    DEBUG = True
    print(
        "[WARN] Debugging mode is enabled. This SHOULD slow down all packet processing operations."
    )
    CFLAGS.append("-DDBUG=1")

BLOCK_CIPHER_MODE = args.mode
print("[INFO] Run AES encryption with %s mode." % BLOCK_CIPHER_MODE)
CFLAGS.append('-D%s=1' % (args.mode.upper()))

ip_route = pyroute2.IPRoute()
# Get index of the egress interface
eg_if_dev_idx = ip_route.link_lookup(ifname=eg_if_dev)[0]

load_st_ts = time.time()
bpf = BPF(src_file="./xdp_udp_aes.c", cflags=CFLAGS)
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

print("[INFO] Time used to compile and load: %s secs"
      % (time.time() - load_st_ts))

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
