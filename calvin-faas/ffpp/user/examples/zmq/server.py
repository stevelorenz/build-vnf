#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Server for slow-path processing.
"""

import time
import json
import sys

import zmq
import docker

if __name__ == "__main__":
    ctx = zmq.Context()
    sock = ctx.socket(zmq.REP)
    sock.bind("tcp://127.0.0.1:9999")

    docker_clt = docker.from_env()
    container = docker_clt.containers.list(
        all=True, filters={"name": "ffpp-dev-interactive"}
    )[0]

    while True:
        try:
            msg = json.loads(sock.recv().decode())
            action = msg["action"]
            if action == "off":
                container.update(cpu_quota=int(100000 * 0.5))
            sock.send("OK".encode())
        except KeyboardInterrupt:
            docker_clt.close()
            sys.exit(0)
