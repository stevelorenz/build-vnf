#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Simple Unix Domain Socket server for test
"""

import os
import socket
import struct
import sys
import time


def main():
    quit = False
    server_address = '/uds_socket'
    meta_data = bytearray(4)
    img_data = bytearray(50000)

    try:
        os.unlink(server_address)
    except OSError:
        if os.path.exists(server_address):
            raise
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.bind(server_address)

    try:
        # main loop
        sock.listen(1)
        print("Waiting for connections")
        conn, client_address = sock.accept()

        vd = memoryview(img_data)
        while(not quit):
            read = 0
            to_read = conn.recv(4)
            to_read = struct.unpack(">I", to_read)[0]
            while to_read > 0:
                # MARK: recv_into does not support offset, so memoryview is used
                read = conn.recv_into(vd[read:], to_read)
                to_read -= read
            print(read)
            time.sleep(0.5)
            struct.pack_into(">I", meta_data, 0, read)
            conn.sendall(meta_data)
            conn.sendall(img_data[:to_read])

    except KeyboardInterrupt:
        quit = True
    finally:
        conn.close()


if __name__ == "__main__":
    main()
