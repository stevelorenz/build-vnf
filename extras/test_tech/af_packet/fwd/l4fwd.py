#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Forwarding forwards and backwards SFC traffic with AF_PACKET
Email: xianglinks@gmail.com
"""

import argparse
import binascii
import logging
import multiprocessing
import socket
import sys

BUFFER_SIZE = 8192  # bytes

#############
#  Logging  #
#############

fmt_str = "%(asctime)s %(levelname)-8s %(processName)s %(message)s"
level = {"INFO": logging.INFO, "DEBUG": logging.DEBUG, "ERROR": logging.ERROR}

logger = logging.getLogger(__name__)
handler = logging.StreamHandler()
formatter = logging.Formatter(fmt_str)
handler.setFormatter(formatter)
logger.addHandler(handler)
logger.setLevel(level["INFO"])


#####################
#  Forward Program  #
#####################


def bind_raw_sock_pair(in_iface, out_iface):
    """Create and bind raw socket pairs"""
    try:
        recv_sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(3))
        send_sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(3))
    except socket.error as error:
        logger.error(error)
        sys.exit(1)

    recv_sock.bind((in_iface, 0))
    send_sock.bind((out_iface, 0))

    return (recv_sock, send_sock)


def forwards_forward(ifce_a, ifce_b):
    logger.info("Start forwards forwarding...")
    recv_sock, send_sock = bind_raw_sock_pair(ifce_a, ifce_b)
    pack_arr = bytearray(BUFFER_SIZE)
    recv_num = 0

    while True:
        pack_len = recv_sock.recv_into(pack_arr, BUFFER_SIZE)
        send_sock.send(pack_arr[0:pack_len])
        recv_num += 1
        logger.debug("[FORWARDS] Receive {} packets".format(recv_num))


def backwards_forward(ifce_a, ifce_b):
    logger.info("Start backwards forwarding...")
    recv_sock, send_sock = bind_raw_sock_pair(ifce_a, ifce_b)
    pack_arr = bytearray(BUFFER_SIZE)

    while True:
        pack_len = recv_sock.recv_into(pack_arr, BUFFER_SIZE)
        send_sock.send(pack_arr[0:pack_len])


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="L4 forwarding with AF_PACKET")
    parser.add_argument("ingress", type=str, help="Ingress interface")
    parser.add_argument("egress", type=str, help="Egress interface")
    parser.add_argument("--log", type=str, default="INFO", help="logging level")
    parser.add_argument(
        "--backwards", action="store_true", help="Enable backwards forwarding."
    )

    args = parser.parse_args()
    logger.setLevel(level[args.log])

    fw_proc = multiprocessing.Process(
        target=forwards_forward, args=(args.ingress, args.egress)
    )
    bw_proc = multiprocessing.Process(
        target=backwards_forward, args=(args.egress, args.ingress)
    )

    fw_proc.start()
    if args.backwards:
        bw_proc.start()

    # MARK: Sockets are not properly released
    fw_proc.join()
    if args.backwards:
        bw_proc.join()
