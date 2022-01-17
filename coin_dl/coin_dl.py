#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Slow Python program to run the image preprocessor
"""

import os
import socket
import time
import sys

import preprocessor

kMaxUDSMTU = 65000


def warm_up(prep):
    print("* Warm-up the preprocessor")
    raw_img_data = prep.read_img_jpeg_bytes("./pedestrain.jpg")
    start = time.time()
    _ = prep.inference(raw_img_data, 70)
    duration = time.time() - start
    print(f"* Warm-up finished. Takes {duration} seconds")


def main_loop(server_addr, client_addr, prep, raw_img_data):
    print(f"* Run UDS server on addr: {server_addr}, client addr: {client_addr}")
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    sock.bind(server_addr)

    try:
        print("* Start main loop")
        while True:
            _ = sock.recvfrom(2 ** 18)[0]
            resp = prep.inference(raw_img_data, 70)
            sock.sendto(resp, client_addr)
    except KeyboardInterrupt:
        print("KeyboardInterrupt detected! Exit program")
        sys.exit(0)


def main():
    prep = preprocessor.Preprocessor()
    # BUG:
    raw_img_data = prep.read_img_jpeg_bytes("./pedestrain.jpg")
    warm_up(prep)

    server_addr = "/tmp/coin_dl_server"
    client_addr = "/tmp/coin_dl_client"

    for addr in [client_addr, server_addr]:
        try:
            os.remove(addr)
        except OSError:
            pass

    main_loop(server_addr, client_addr, prep, raw_img_data)


if __name__ == "__main__":
    main()
