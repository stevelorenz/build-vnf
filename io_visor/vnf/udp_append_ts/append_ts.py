#! /usr/bin/env python2
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Receive UDP packets from SFC and append timestamps by using eBPF for
       frame filtering.
       - Ingress packets are filtered with eBPF
       - The python program modify the UDP payload and update the MAC addresses,
       then packet are sent to the egress interface with packet socket.

MARK : No queuing in the application, assume packets are not dropped by the
       kernel socket buffer

Email: xianglinks@gmail.com
"""
from __future__ import print_function

import argparse
import binascii
import socket
import sys
import struct
import time

from bcc import BPF

BUFFER_SIZE = 1500

# Header lengths in bytes
ETH_HDL = 14
UDP_HDL = 8

# MAC address of source and destination instances in the SFC
SRC_MAC = 'fa:16:3e:04:07:36'
DST_MAC = 'fa:16:3e:58:25:fb'
SRC_IP = '10.0.0.13'
DST_IP = '10.0.0.15'
SRC_MAC_B = binascii.unhexlify(SRC_MAC.replace(':', ''))
DST_MAC_B = binascii.unhexlify(DST_MAC.replace(':', ''))
SRC_IP_B = struct.pack('!4B', *[int(x) for x in SRC_IP.split('.')])
DST_IP_B = struct.pack('!4B', *[int(x) for x in DST_IP.split('.')])
MAC_LEN = len(DST_MAC_B)

print('SRC MAC: %s' % SRC_MAC)
print('DST MAC: %s' % DST_MAC)

print('SRC IP: %s' % SRC_IP)
print('DST IP: %s' % DST_IP)


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
        s = ((s+w) & 0xffff) + ((s+w) >> 16)

    return ~s & 0xffff


def calc_udp_cksum(data):
    """Calculate UDP header checksum"""
    checksum = 0
    data_len = len(data)
    if (data_len % 2) == 1:
        data_len += 1
        data += struct.pack('!B', 0)

    for i in range(0, len(data), 2):
        w = (data[i] << 8) + (data[i + 1])
        checksum += w

    checksum = (checksum >> 16) + (checksum & 0xFFFF)
    checksum = ~checksum & 0xFFFF

    return checksum


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

    # --- Ingress Interface ---

    # Load BPF program, written in C code
    bpf = BPF(src_file="./udp_filter.c", debug=0)
    filter_func = bpf.load_func("udp_filter", BPF.SOCKET_FILTER)

    # Bind a new raw socket to the ingress interface
    BPF.attach_raw_socket(filter_func, in_ifce)
    print('[INFO] Attach eBPF raw socket to interface: %s' % in_ifce)

    recv_sock_fd = filter_func.sock
    recv_sock = socket.fromfd(recv_sock_fd, socket.AF_PACKET, socket.SOCK_RAW,
                              socket.htons(3))
    recv_sock.setblocking(True)

    # --- Egress Interface ---

    send_sock = socket.socket(
        socket.AF_PACKET, socket.SOCK_RAW, socket.htons(3))
    send_sock.bind((eg_ifce, 0))
    print('[INFO] Bind send socket to interface: %s' % eg_ifce)

    # --- Frame Processing ---

    eth_frame = bytearray(BUFFER_SIZE)

    # Recv loop
    # MARK: Looks same with my raw socket implementation in the DA
    print('[INFO] Enter main looping...')
    try:
        while True:
            # MARK: Use blocking IO
            frame_len = recv_sock.recv_into(eth_frame, BUFFER_SIZE)
            # Get time stamp in microseconds.
            recv_time_b = struct.pack('!Q', int(time.time() * 1e6))
            ts_len = len(recv_time_b)
            # Append time stamps
            hd_offset = 0
            hd_offset += ETH_HDL  # move to IP header
            # Check IP version and calc header length
            ver_ihl = struct.unpack('>B', eth_frame[hd_offset:hd_offset + 1])[0]
            ihl = 4 * int(hex(ver_ihl)[-1])
            # IP total length
            old_ip_tlen = struct.unpack(
                '>H', eth_frame[hd_offset + 2:hd_offset + 4])[0]
            hd_offset += ihl  # move to UDP header
            udp_pl_offset = hd_offset + UDP_HDL

            # UDP payload length
            old_udp_pl_len = struct.unpack(
                '>H', eth_frame[hd_offset + 4:hd_offset + 6]
            )[0] - UDP_HDL

            # Append recv time stamp at end
            eth_frame[udp_pl_offset + old_udp_pl_len:udp_pl_offset +
                      old_udp_pl_len + ts_len] = recv_time_b

            # Set UDP and IP total length with new payload
            new_udp_tlen = struct.pack(
                '>H', (old_udp_pl_len + UDP_HDL + ts_len),
            )
            eth_frame[hd_offset + 4:hd_offset + 6] = new_udp_tlen

            # MARK: Recalculate the UDP checksum
            #       Assume that the SRC_IP and DST_IP are always not modified by
            #       the SFC
            # TODO: Such information SHOULD provided by BPF
            # Get UDP pseudo header: src_ip, dst_ip, zero, protocol, udp_length
            # Set checksum to zero
            eth_frame[hd_offset + 6:hd_offset + 8] = struct.pack('>H', 0)
            # Create the pseudo UDP header
            pseudo_udp_header = struct.pack('!BBH', 0, socket.IPPROTO_UDP,
                                            old_udp_pl_len + ts_len + UDP_HDL)
            pseudo_udp_header = SRC_IP_B + DST_IP_B + pseudo_udp_header
            # src_port, dst_port, udp_length, 0
            udp_header = eth_frame[hd_offset:hd_offset + 8]
            udp_cksum = calc_udp_cksum(pseudo_udp_header+udp_header+eth_frame[
                udp_pl_offset:udp_pl_offset+old_udp_pl_len + ts_len])
            eth_frame[hd_offset + 6:hd_offset +
                      8] = struct.pack('>H', udp_cksum)

            # Move back to IP header
            hd_offset -= ihl
            new_ip_tlen = struct.pack(
                '>H', (old_ip_tlen + ts_len)
            )
            eth_frame[hd_offset + 2:hd_offset + 4] = new_ip_tlen

            # Set checksum field to zero
            eth_frame[hd_offset + 10:hd_offset + 12] = struct.pack('>H', 0)
            new_iph_cksum = calc_ih_cksum(
                eth_frame[hd_offset:hd_offset + ihl]
            )
            eth_frame[hd_offset + 10:hd_offset +
                      12] = struct.pack('<H', new_iph_cksum)
            frame_len += ts_len
            eth_frame[0:MAC_LEN] = DST_MAC_B
            eth_frame[MAC_LEN:MAC_LEN * 2] = SRC_MAC_B
            send_sock.send(eth_frame[0: frame_len])

    except KeyboardInterrupt:
        print("[INFO] KeyboardInterrupt detected, exits...")
        recv_sock.close()
        send_sock.close()
        sys.exit(0)


if __name__ == '__main__':
    main()
