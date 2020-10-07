#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Server for slow-path processing.
"""

import argparse
import sys
import time

import zmq

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="")
    args = parser.parse_args()

    ctx = zmq.Context()
    sock = ctx.socket(zmq.REP)
    sock.bind("ipc:///tmp/ffpp.sock")

    while True:
        try:
            msg = sock.recv().decode()
            sock.send(msg[:32].encode())
        except KeyboardInterrupt:
            ctx.destroy()
            sys.exit(0)
