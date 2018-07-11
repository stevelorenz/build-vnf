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
XOR_OFFSET = 16
# MARK: Current XOR_OFFSET, the alignment offset is const 2 bytes
ALIGN_OFFSET = 2

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

    parser.add_argument('-p', type=int,
                        help='Flag for UDP operation.\n' +
                        '  0: Forwarding. 2: XOR payload')

    parser.add_argument('-i', type=str,
                        help='Interface(s) name list. Delimiter=,'
                        )

    parser.add_argument('--src_mac', type=str, default="",
                        help='Source MAC address for chaining. Delimiter=:')
    parser.add_argument('--dst_mac', type=str, default="",
                        help='Destination MAC address')
    parser.add_argument('--payload_size', type=int, default=128,
                        help='UDP payload size in bytes.')
    parser.add_argument('--debug', action='store_true',
                        help='Enable debugging mode.')
    args = parser.parse_args()

    UDP_PAYLOAD_SIZE = args.payload_size

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

    print('[INFO] XOR offset: %d' % XOR_OFFSET)

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
        with open("./xdp_udp_forwarding.c", "r") as src_file:
            src_code = src_file.read()
        src_code = src_code.replace('SRC_DST_MAC_INIT', src_dst_mac_init, 1)
        bpf = BPF(text=src_code, cflags=CFLAGS)
        print('[INFO] CFlags: ')
        print(CFLAGS)
        init_xdp_progs(bpf, ifce_lst, func_names)

    elif opt == 2:
        CFLAGS.extend(["-include", "../../../shared_lib/random.h"])
        CFLAGS.append("-DXOR_IFCE=0")
        CFLAGS.append("-DXOR_OFFSET=%s" % XOR_OFFSET)
        print('[INFO] CFlags: ')
        print(CFLAGS)
        with open("./xdp_udp_xor.c", "r") as src_file:
            src_code = src_file.read()

        # MARK: Really do not want to do this... ugly code here...
        # BUT It works!

        XOR_SIZE = UDP_PAYLOAD_SIZE - XOR_OFFSET
        if XOR_SIZE <= 0:
            raise RuntimeError('The XOR size is not bigger than zero!')

        num, rmd = divmod(XOR_SIZE - ALIGN_OFFSET, 8)
        xor_opt_code = ""
        xor_opt_code += """
        if ((pt_pload_8 + sizeof(pt_pload_8) <= data_end)) {
        *pt_pload_8 = (*pt_pload_8 ^ 0x03);
        pt_pload_8 += 1;
        }
        """ * (ALIGN_OFFSET + rmd)

        xor_opt_code += """
        pt_pload_64 = (uint64_t *)pt_pload_8;
        """
        xor_opt_code += """
        if ((pt_pload_64 + sizeof(pt_pload_64) <= data_end)) {
        *pt_pload_64 = (*pt_pload_64 ^ 0x%s);
        pt_pload_64 += 1;
        }
        """ % ('03' * 8) * (num)

        print('[INFO] Align: %dB, Remainder: %dB, Num8B: %d'
              % (ALIGN_OFFSET, rmd, num))

        src_code = src_code.replace('XOR_OPT_CODE', xor_opt_code, 1)
        src_code = src_code.replace('SRC_DST_MAC_INIT', src_dst_mac_init, 1)
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
