#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: XDP Firewall Frontend

Email: xianglinks@gmail.com
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

CFLAGS = ["-w", "-Wall", "-O3"]


def init_xdp_progs(bpf, ifce_names, func_names):
    st_ts = time.time()
    for ifce, func in zip(ifce_names, func_names):
        print(
            '[INFO] Load and attach XDP func %s to interface %s' % (func, ifce))
        ld_fn = bpf.load_func(func, BPF.XDP)
        bpf.attach_xdp(ifce, ld_fn, 0)
    dur = time.time() - st_ts
    print('[INFO] XDP init duration: %f sec(s)' % dur)


def cleanup_xdp_progs(bpf, ifce_names):
    for ifce in ifce_names:
        print('[INFO] Remove XDP programs from interface %s' % ifce)
        bpf.remove_xdp(ifce, flags=0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='UDP segments processing with XDP')

    parser.add_argument('-i', type=str,
                        help='Interface(s) name list. Delimiter=,'
                        )
    parser.add_argument('--debug', action='store_true',
                        help='Enable debugging mode.')

    parser.add_argument('--src_mac', type=str, default="",
                        help='Source MAC address for chaining. Delimiter=:')
    parser.add_argument('--dst_mac', type=str, default="",
                        help='Destination MAC address')
    parser.add_argument('-l', '--payload_size', type=int, default=256,
                        help="UDP payload size, used to filter out UDP segments"
                        + " with different payload size")

    args = parser.parse_args()

    # Substitute SRC and DST MAC
    if args.src_mac or args.dst_mac:
        print('[INFO] Substitute SRC and DST MAC in the C code.')
        src_mac = args.src_mac.split(':')
        dst_mac = args.dst_mac.split(':')
    else:
        print('[INFO] Use default SRC and DST MAC for debugging')
        src_mac = ['08', '00', '27', 'd6', '69', '61']
        dst_mac = ['08', '00', '27', 'e1', 'f1', '7d']
    src_mac = map(lambda x: '0x'+x, src_mac)
    dst_mac = map(lambda x: '0x'+x, dst_mac)
    src_dst_mac_init = """
        uint8_t src_mac[ETH_ALEN] = {%s};
        uint8_t dst_mac[ETH_ALEN] = {%s};
    """ % (', '.join(src_mac), ', '.join(dst_mac))

    if args.debug:
        DEBUG = True
        print(
            "[WARN] Debugging mode is enabled. This SHOULD slow down all packet processing operations."
        )
        CFLAGS.append("-DDEBUG=1")

    CFLAGS.append("-DUDP_PAYLOAD_LEN={}".format(args.payload_size))
    print("[INFO] Allowed UDP Payload size: {} Bytes".format(args.payload_size))

    ifce_lst = map(str.strip, args.i.split(','))
    XDP_ACTION = 'BOUNCE'
    if len(ifce_lst) == 1:
        print('[INFO] Single interface, XDP TX is used.')
        func_names = ('ingress_xdp_redirect', )
    elif len(ifce_lst) == 2:
        print('[INFO] Ingress interface: %s, egress interface: %s'
              % (ifce_lst[0], ifce_lst[1]))
        XDP_ACTION = 'REDIRECT'
        func_names = ('ingress_xdp_redirect', 'egress_xdp_tx')
    else:
        raise RuntimeError('Invalid interface number. SHOULD be 1-2.')
    CFLAGS.append("-DXDP_ACTION=%s" % XDP_ACTION)

    print('[INFO] Operation: Forwarding')
    with open("./xdp_firewall.c", "r") as src_file:
        src_code = src_file.read()
    src_code = src_code.replace('SRC_DST_MAC_INIT', src_dst_mac_init, 1)
    bpf = BPF(text=src_code, cflags=CFLAGS)
    print('[INFO] CFlags: ')
    print(CFLAGS)
    init_xdp_progs(bpf, ifce_lst, func_names)

    if len(ifce_lst) == 2:
        print('[INFO] Add egress interface to BPF_DEVMAP')
        ip_route = pyroute2.IPRoute()
        eg_if_dev_idx = ip_route.link_lookup(ifname=ifce_lst[1])[0]
        tx_port = bpf.get_table("tx_port")
        tx_port[0] = ct.c_int(eg_if_dev_idx)

    # Start main loop
    udp_nb_map = bpf["udp_nb_map"]
    # filter_out_nb_map = bpf["filter_out_nb_map"]
    print("Start main loop, hit CTRL+C to stop")
    while True:
        try:
            if DEBUG:
                udp_nb = udp_nb_map.sum(0).value
                print("TS: {}, Number of valid UDP segments: {}".format(
                    time.time(), udp_nb)
                )
            time.sleep(0.5)
        except KeyboardInterrupt:
            break

    cleanup_xdp_progs(bpf, ifce_lst)
