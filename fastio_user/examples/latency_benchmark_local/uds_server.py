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
            data_len = 0
            read = 0
            conn.recv_into(meta_data, 4)
            to_read = struct.unpack_from(">L", meta_data, 0)[0]
            while to_read > 0:
                # MARK: recv_into does not support offset, so memoryview is used
                read = conn.recv_into(vd[read:], to_read)
                data_len += read
                to_read -= read
            time.sleep(0.5)
            print(data_len)
            # Only send 50% back
            data_len = int(data_len / 2)
            struct.pack_into(">L", meta_data, 0, data_len)
            conn.sendall(meta_data)
            conn.sendall(img_data[:data_len])

    except KeyboardInterrupt:
        quit = True
    finally:
        conn.close()


if __name__ == "__main__":
    main()
