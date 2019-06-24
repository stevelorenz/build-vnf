#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

#
"""
About: Service CLassifier (SCL)

       This is exposed to the clients as the service endpoint for services
       extended with SFCs.
       This is the first ingress classifier in the SFC, it should add the NSH
       header with proper metadata based on the classification rules.

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

import nsh

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
logger.setLevel(level["DEBUG"])

###############
#  Constants  #
###############

UDP_HDL = 8

MAC_WHITE_LIST = [
    "00:00:00:00:00:02",
    "00:00:00:00:08:00",
]

MAC_WHITE_LIST_B = [binascii.unhexlify(
    mac.replace(":", "")) for mac in MAC_WHITE_LIST]

MAC_ADDR_LEN = len(MAC_WHITE_LIST_B[0])

PORT_SERVICE_TABLE = {
    6666: "10.0.0.200"
}


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
    # Allocate an bytes array for storing received frames
    # This is used to avoid allocate a new object each time by using Python
    # MARK: Instead of using 3rd party libraries for packet parsing, all
    # operations here are peformed directly on the bytearray to avoid the
    # latency of using additional objects.
    frame_arr = bytearray(BUFFER_SIZE)
    logger.info("Entering IO loop.")
    while True:
        # Receive a frame
        frame_len = sock.recv_into(frame_arr, BUFFER_SIZE)
        hd_offset = 0
        eth_typ = struct.unpack(">H", frame_arr[12:14])[0]
        logger.debug("Recv a frame with length: {}, type: {}".format(frame_len,
                                                                     hex(eth_typ)))
        # Check if src MAC is in the whitelist
        dst_mac_b = frame_arr[0:MAC_ADDR_LEN]
        src_mac_b = frame_arr[6:6+MAC_ADDR_LEN]
        if src_mac_b not in MAC_WHITE_LIST_B:
            logger.debug(
                "The source MAC address is not in the white list, source mac:%s, destination mac: %s",
                ":".join("%02x" % b for b in src_mac_b),
                ":".join("%02x" % b for b in dst_mac_b)
            )
            continue

        # ARP
        if eth_typ == int("0x806", 16):
            pass

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

            if proto == 1:
                hd_offset += ihl
                typ, code = struct.unpack(">BB", frame_arr[hd_offset:
                                                           hd_offset+2])
                logger.debug("Recv a ICMP message, type: %d, code: %d",
                             typ, code)

            # UDP protocol
            if proto == 17:
                hd_offset += ihl
                dst_port = struct.unpack(">H", frame_arr[hd_offset + 2:
                                                         hd_offset + 4])[0]
                logger.debug("Recv a UDP segement, destination port: %d",
                             dst_port)

                srv_ip = PORT_SERVICE_TABLE.get(dst_port, None)
                if srv_ip:
                    logger.debug("Port in the service table, server ip:%s",
                                 srv_ip)

                    # Insert the NSH header


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
