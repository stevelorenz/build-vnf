#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: UDP segments processing with XDP
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

UDP_PAYLOAD_SIZE = 128

CFLAGS = ["-w", "-Wall"]


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

    parser.add_argument('-p', type=int,
                        help='Flag for UDP operation.\n' +
                        '  0: Forwarding. 2: XOR payload')

    parser.add_argument('-i', type=str,
                        help='Interface(s) name list. Delimiter=,'
                        )

    parser.add_argument('--debug', action='store_true',
                        help='Enable debugging mode.')
    args = parser.parse_args()

    if args.debug:
        DEBUG = True
        print(
            "[WARN] Debugging mode is enabled. This SHOULD slow down all packet processing operations."
        )
        CFLAGS.append("-DDEBUG=1")

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

    opt = args.p
    if opt == 0:
        print('[INFO] Operation: Forwarding')
        bpf = BPF(src_file="./xdp_udp_forwarding.c", cflags=CFLAGS)
        print('[INFO] CFlags: ')
        print(CFLAGS)
        init_xdp_progs(bpf, ifce_lst, func_names)

    elif opt == 2:
        CFLAGS.extend(["-include", "../../../shared_lib/random.h"])
        CFLAGS.append("-DXOR_IFCE=0")

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
        init_xdp_progs(bpf, ifce_lst, func_names)
    else:
        raise RuntimeError('Invalid UDP segment operation.')

    if len(ifce_lst) == 2:
        print('[INFO] Add egress interface to BPF_DEVMAP')
        ip_route = pyroute2.IPRoute()
        eg_if_dev_idx = ip_route.link_lookup(ifname=ifce_lst[1])[0]
        tx_port = bpf.get_table("tx_port")
        tx_port[0] = ct.c_int(eg_if_dev_idx)

    # Start main loop
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

    cleanup_xdp_progs(bpf, ifce_lst)
