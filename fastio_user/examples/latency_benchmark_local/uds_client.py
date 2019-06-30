#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Simple Unix Domain Socket client for test
       Performance is NOT considered
"""

import socket
import sys
import os
import time
import struct

if __name__ == "__main__":
    server_address = '/uds_socket'
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        sock.connect(server_address)
    except socket.error as msg:
        print(msg)
        sys.exit(1)

    for i in range(10):
        img_path = "./pikachu.jpg"
        with open(img_path, 'rb') as f:
            data_send = f.read()
        sock.sendall(struct.pack('>L', len(data_send)) + data_send)
        print("idx: {}, image {} is sent, length in bytes: {}.".format(
            i, img_path, len(data_send)))

        to_read = sock.recv(4)
        to_read = struct.unpack(">L", to_read)[0]
        print("- Length of returned bytes: %d" % to_read)
        _ = sock.recv(to_read)

    sock.close()
