#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""

About: XOR UDP payload
Email: xianglinks@gmail.com

Ref  : - bcc/example/networking/xdp
           - xdp_drop_count.py
           - xdp_redirect_map.py
"""

import argparse
import ctypes as ct
import os
import random
import sys
import time

from bcc import BPF

import pyroute2

DEBUG = False

MAX_RAND_BYTES_LEN = 512

CFLAGS = ["-w", "-Wall"]
# Add external headers
CFLAGS.extend(["-include", "../../../shared_lib/random.h"])

parser = argparse.ArgumentParser(description='XOR UDP payload.')
parser.add_argument('-i', metavar='IFACE', type=str,
                    help='Ingress interface name. e.g. eth1')
parser.add_argument('-e', metavar='IFACE', type=str,
                    help='Egress interface name.')
parser.add_argument('--size', type=int, default=128,
                    help='UDP payload size in bytes. Maximal 128 bytes.')
parser.add_argument('--debug', action='store_true',
                    help='Enable debugging mode.')
args = parser.parse_args()

UDP_PAYLOAD_SIZE = args.size
print('[INFO] UDP payload size: {} bytes'.format(UDP_PAYLOAD_SIZE))
in_if_dev = args.i
eg_if_dev = args.e

if args.debug:
    DEBUG = True
    print(
        "[WARN] Debugging mode is enabled. This SHOULD slow down all packet processing operations."
    )
    CFLAGS.append("-DDEBUG=1")

CFLAGS.append("-DXOR_IFCE=0")

print('[INFO] CFlags: ')
print(CFLAGS)

# Generate random bytes for XOR
rand_bytes = bytearray(os.urandom(MAX_RAND_BYTES_LEN))

ip_route = pyroute2.IPRoute()
# Get index of the egress interface
eg_if_dev_idx = ip_route.link_lookup(ifname=eg_if_dev)[0]
load_st_ts = time.time()

with open("./xdp_udp_xor.c", "r") as src_file:
    src_code = src_file.read()
# MARK: Really do not want to do this... ugly code here...
# BUT It works!
if UDP_PAYLOAD_SIZE > 0:
    repeat_opt = """
            if ((pt_pload + sizeof(pt_pload) <= data_end)) {
                    *pt_pload = (*pt_pload ^ 0x3);
                    pt_pload += 1;
            }
    """ * (UDP_PAYLOAD_SIZE)
else:
    repeat_opt = ""
src_code = src_code.replace('REPEAT_OPT', repeat_opt, 1)

bpf = BPF(text=src_code, cflags=CFLAGS)

# Add egress interface index into the DEV map
tx_port = bpf.get_table("tx_port")
tx_port[0] = ct.c_int(eg_if_dev_idx)

xor_bytes_map = bpf.get_table("xor_bytes_map")
for i in range(MAX_RAND_BYTES_LEN):
    xor_bytes_map[i] = ct.c_int(rand_bytes[i])

# Attach XDP functions
in_fn = bpf.load_func("ingress_xdp_redirect", BPF.XDP)
eg_fn = bpf.load_func("egress_xdp_tx", BPF.XDP)
print("Attach XDP filter to ingress device: %s" % in_if_dev)
print("Attach XDP filter to egress device: %s" % eg_if_dev)
bpf.attach_xdp(in_if_dev, in_fn, 0)
bpf.attach_xdp(eg_if_dev, eg_fn, 0)

print("[INFO] Time used to compile and load: %s secs"
      % (time.time() - load_st_ts))

udp_nb_map = bpf["udp_nb_map"]
print("Start main loop, hit CTRL+C to stop")
while True:
    try:
        if DEBUG:
            udp_nb = udp_nb_map.sum(0).value
            print("Number of received UDP segment: {}".format(udp_nb))
        time.sleep(0.5)
    except KeyboardInterrupt:
        break
print("Remove XDP filter from device: %s" % in_if_dev)
bpf.remove_xdp(in_if_dev, flags=0)
print("Remove XDP filter from device: %s" % eg_if_dev)
bpf.remove_xdp(eg_if_dev, flags=0)
