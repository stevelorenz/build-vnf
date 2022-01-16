#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Slow Python program to run the image preprocessor
"""

import os
import socket
import time

import preprocessor

kMaxUDSMTU = 65000


def warm_up(prep):
    print("* Warm-up the preprocessor")
    raw_img_data = prep.read_img_jpeg_bytes("./pedestrain.jpg")
    start = time.time()
    _ = prep.inference(raw_img_data, 70)
    duration = time.time() - start
    print(f"* Warm-up finished. Takes {duration} seconds")


def main_loop():
    pass


def main():
    prep = preprocessor.Preprocessor()
    # BUG:
    raw_img_data = prep.read_img_jpeg_bytes("./pedestrain.jpg")
    warm_up(prep)

    server_addr = "/tmp/coin_dl_server"
    client_addr = "/tmp/coin_dl_client"
    print(f"* Run UDS server on addr: {server_addr}")

    for addr in [client_addr, server_addr]:
        try:
            os.remove(addr)
        except OSError:
            pass

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    sock.bind(server_addr)

    data = sock.recvfrom(2 ** 18)[0]
    print(len(data))
    resp = prep.inference(raw_img_data, 70)
    sock.sendto(resp, client_addr)


if __name__ == "__main__":
    main()
