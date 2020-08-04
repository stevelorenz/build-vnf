#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Server for slow-path processing.
"""

import argparse
import time
import json
import sys

import zmq
import docker

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="ZMQ server for Docker management.")
    parser.add_argument(
        "--no_update", action="store_true", help="Do not perform any update operations."
    )

    args = parser.parse_args()
    if args.no_update:
        print("Do not perform any update operations.")

    ctx = zmq.Context()
    sock = ctx.socket(zmq.REP)
    sock.bind("ipc:///tmp/ffpp.sock")

    docker_clt = docker.from_env()
    container = docker_clt.containers.list(
        all=True, filters={"name": "vnf"}
    )[0]

    while True:
        try:
            msg: str = json.loads(sock.recv().decode())
            action = msg["action"]
            if action == "off" and not args.no_update:
                # MARK: This action takes more than 50ms on my laptop (Core
                # i7-7600U 2.8GHz).
                container.update(cpu_quota=0)
            elif action == "on" and not args.no_update:
                container.update(cpu_quota=-1)
            sock.send("OK".encode())
        except KeyboardInterrupt:
            docker_clt.close()
            ctx.destroy()
            sys.exit(0)
