#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

#
"""
About: An example of using raw socket (with AF_PACKET) for forwarding layer 2 Ethernet frames
       Run this script INSIDE the virtual instance. (e.g. Mininet host or
       Container net docker container)

       - This is used as the elementary function to handle SFC traffic if layer 3
         addresses are not modified.

       - Extend the processing part to build your own network function

       - Code is written with Python3 (at least 3.4)
MARK :
       - Use multiprocessing

Email: xianglinks@gmail.com
"""

import binascii
import logging
import os
import socket
import struct
import sys
import time
from multiprocessing import Process, Queue, active_children

BUFFER_SIZE = 2048
IO_SLEEP = 0.001  # 1ms

# Header lengths in bytes
ETH_HDL = 14
UDP_HDL = 8

#############
#  Logging  #
#############

fmt_str = "%(asctime)s %(levelname)-8s %(processName)s %(message)s"
level = {
    "INFO": logging.INFO,
    "DEBUG": logging.DEBUG,
    "ERROR": logging.ERROR
}

logger = logging.getLogger(__name__)
handler = logging.StreamHandler()
formatter = logging.Formatter(fmt_str)
handler.setFormatter(formatter)
logger.addHandler(handler)
logger.setLevel(level["INFO"])


##############
#  NF Funcs  #
##############

def calc_ih_cksum(hd_b_arr):
    """Calculate IP header checksum
    MARK: To generate a new checksum, the checksum field itself is set to zero

    :para hd_b_arr: Bytes array of IP header
    :retype: int
    """
    s = 0
    for i in range(0, len(hd_b_arr), 2):
        a, b = struct.unpack('>2B', hd_b_arr[i:i + 2])
        w = a + (b << 8)
        s = ((s + w) & 0xffff + (s+w >> 16))
    return ~s & 0xffff


def io_loop(sock):
    """Main IO looping
    TODO: Currently show how to receive a Ethernet frame from the interface and
    check if it is a ICMP message. Extend this to receive UDP segements and
    modify the payload with your processing function. The UDP and IP checksums
    should be recalculated after the payload modification: The UDP checksum can
    be set to zero to disable it. The calculation of IP checksum can be
    performed either with helper functions listed above or using OVS TX checksum
    offloading.

    :param sock:
    """
    # Allocate an bytes array for storing received frames
    # This is used to avoid allocate a new object each time by using Python
    icmp_count = 0
    frame_arr = bytearray(BUFFER_SIZE)
    logger.info("Entering IO loop.")
    while True:
        # Receive a frame
        frame_len = sock.recv_into(frame_arr, BUFFER_SIZE)
        logger.debug("Recv a frame with length: {}".format(frame_len))

        # Parse frame
        hd_offset = 0
        eth_typ = struct.unpack(">H", frame_arr[12:14])[0]
        # IPv4 packets 0x800
        if eth_typ == int("0x800", 16):
            hd_offset += ETH_HDL
            # Calc IP header length
            ver_ihl = struct.unpack(
                ">B", frame_arr[hd_offset:hd_offset + 1])[0]
            ihl = 4 * int(hex(ver_ihl)[-1])
            orig_ip_tlen = struct.unpack(
                ">H", frame_arr[hd_offset + 2:hd_offset + 4])[0]
            proto = struct.unpack(
                ">B", frame_arr[hd_offset + 9:hd_offset + 10])[0]
            logger.debug(
                "Recv a IPv4 packet, header len: {}, total len: {}, proto: {}".format(
                    ihl, orig_ip_tlen, proto)
            )
            # ICMP protocol
            if proto == 1:
                icmp_count += 1
                logger.info(
                    "Receive a ICMP packet! Total ICMP count: {}".format(icmp_count))


def proc_loop():
    """TODO: Use another process for processing

    The data communication between processes can use multiprocessing.Queue() or
    Pipe() objects
    """
    pass


##########
#  Main  #
##########

if __name__ == "__main__":
    try:
        logger.info("Create raw sockets\n")
        # Create a raw socket to receive and send packets, the protocol number 3
        # means receive all types of Ethernet frames.
        sock = socket.socket(
            socket.AF_PACKET, socket.SOCK_RAW, socket.htons(3))
    except socket.error as error:
        raise error

    # Get the interface name, assumed there is ONLY one interface
    ifce = [i for i in os.listdir("/sys/class/net") if i != "lo"][0]
    logger.info("Bind the socket to the interface: {}".format(ifce))
    sock.bind((ifce, 0))

    # TODO: Use two processes for IO and processing to enable caching and latter
    # multi-core processing (use Linux taskset or Python psutil)
    # Currently IO and processing are performed by the same process
    io_proc = Process(target=io_loop, args=(sock, ))
    io_proc.start()

    try:
        while True:
            time.sleep(0.5)
    except KeyboardInterrupt:
        logger.info("Keyboard interrupt detected, exit.")
        logger.info("Kill all sub-processes.")
        for proc in active_children():
            proc.terminate()
    finally:
        logger.info("Free resources")
        sock.close()
