#! /usr/bin/env python2
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Receive UDP packets from SFC and append timestamps by using eBPF for
       frame filtering.

Email: xianglinks@gmail.com
"""
from __future__ import print_function

import argparse
import os
import socket
import sys
import time

from bcc import BPF

BUFFER_SIZE = 1500


def main():
    """Main loop"""
    # Parse args
    parser = argparse.ArgumentParser(
        description='Append timestamps to all UDP packets received from SFC.',
        formatter_class=argparse.RawTextHelpFormatter
    )

    parser.add_argument('--in_ifce', help="Ingress interface.", default='eth1')
    parser.add_argument('--eg_ifce', help="Egress interface.", default='eth2')

    args = parser.parse_args()
    in_ifce = args.in_ifce
    eg_ifce = args.eg_ifce

    # Load BPF program, written in C code
    bpf = BPF(src_file="./udp_filter.c", debug=1)
    filter_func = bpf.load_func("udp_filter", BPF.SOCKET_FILTER)

    # Bind a new raw socket to the ingress interface
    BPF.attach_raw_socket(filter_func, in_ifce)

    recv_sock_fd = filter_func.sock
    recv_sock = socket.fromfd(recv_sock_fd, socket.AF_PACKET, socket.SOCK_RAW,
                              socket.htons(3))
    recv_sock.setblocking(True)

    eth_frame = bytearray(BUFFER_SIZE)

    # Recv loop
    # MARK: Looks same with my raw socket implementation in the DA
    try:
        while True:
            frame_len = recv_sock.recv_into(eth_frame, BUFFER_SIZE)
            print("Receive a frame of the lengh: %d" % frame_len)
            time.sleep(0.01)
    except KeyboardInterrupt:
        print("KeyboardInterrupt detected, exits...")
        recv_sock.close()
        sys.exit(0)


if __name__ == '__main__':
    main()
