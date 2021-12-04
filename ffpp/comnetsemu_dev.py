#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: ONLY for development and test.
"""

import os
import sys

from comnetsemu.cli import CLI
from comnetsemu.net import Containernet
from mininet.log import setLogLevel

CURRENT_DIR = os.path.abspath(os.path.curdir)

with open("./VERSION", "r", encoding="utf-8") as vfile:
    FFPP_VER = vfile.read().strip()

if __name__ == "__main__":
    if os.geteuid() != 0:
        print("Run this script with sudo.", file=sys.stderr)
        sys.exit(1)
    setLogLevel("error")

    net = None
    try:
        net = Containernet(xterms=False)
        dev = net.addDockerHost(
            "dev",
            dimage=f"ffpp:{FFPP_VER}",
            ip="10.0.1.11/16",
            docker_args={
                "hostname": "dev",
                "volumes": {
                    "/dev": {"bind": "/dev", "mode": "rw"},
                    CURRENT_DIR: {"bind": "/ffpp", "mode": "rw"},
                },
                "working_dir": "/ffpp",
            },
        )
        net.start()
        CLI(net)
    finally:
        if net:
            net.stop()
