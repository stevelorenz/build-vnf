#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

import os
import sys

from comnetsemu.cli import CLI
from comnetsemu.net import Containernet
from mininet.log import setLogLevel

CURRENT_DIR = os.path.abspath(os.path.curdir)

if __name__ == "__main__":
    if os.geteuid() != 0:
        print("Run this script with sudo.", file=sys.stderr)
        sys.exit(1)
    setLogLevel("error")
    try:
        net = Containernet(
            xterms=False,
        )

        dev = net.addDockerHost(
            "dev",
            dimage="coin_yolo:0.0.1",
            ip="10.0.1.11/16",
            docker_args={
                "hostname": "dev",
                "volumes": {
                    "/dev": {"bind": "/dev", "mode": "rw"},
                    CURRENT_DIR: {"bind": "/coin_yolo/host", "mode": "rw"},
                },
                "working_dir": "/coin_yolo",
            },
        )
        net.start()
        CLI(net)
    finally:
        net.stop()
