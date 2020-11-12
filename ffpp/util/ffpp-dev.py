#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: FFPP library Docker container development environment utility.
"""

import argparse
import os
import shlex
import subprocess
import sys
import time

from pathlib import Path

import docker

with open("../VERSION", "r") as vfile:
    FFPP_VER = vfile.read().strip()

PARENT_DIR = os.path.abspath(os.path.join(os.path.curdir, os.pardir))


class bcolors:
    HEADER = "\033[95m"
    OKBLUE = "\033[94m"
    OKGREEN = "\033[92m"
    WARNING = "\033[93m"
    FAIL = "\033[91m"
    ENDC = "\033[0m"
    BOLD = "\033[1m"
    UNDERLINE = "\033[4m"


FFPP_DEV_CONTAINER_OPTS_DEFAULT = {
    "auto_remove": True,
    "detach": True,  # -d
    "init": True,
    # I know --privileged is a bad/danger option. It is just used for tests.
    "privileged": True,
    "tty": True,  # -t
    "stdin_open": True,  # -i
    "volumes": {
        "/sys/bus/pci/drivers": {"bind": "/sys/bus/pci/drivers", "mode": "rw"},
        "/sys/kernel/mm/hugepages": {"bind": "/sys/kernel/mm/hugepages", "mode": "rw"},
        "/sys/devices/system/node": {"bind": "/sys/devices/system/node", "mode": "rw"},
        "/dev": {"bind": "/dev", "mode": "rw"},
        "/sys/fs/bpf/": {"bind": "/sys/fs/bpf", "mode": "rw"},
        "/var/run/docker.sock": {"bind": "/var/run/docker.sock", "mode": "rw"},
        PARENT_DIR: {"bind": "/ffpp", "mode": "rw"},
    },
    "working_dir": "/ffpp",
    "name": "ffpp-dev-vnf",
    "command": "bash",
}


def build_image():
    print(
        bcolors.HEADER
        + "ACTION: Build the Docker image for FFPP development."
        + bcolors.ENDC
    )
    if FFPP_VER == "experimental":
        dockerfile = Path("../Dockerfile.experimental")
    else:
        dockerfile = Path("../Dockerfile")
    if not dockerfile.is_file():
        print("Can not find the Dockerfile in path: %s." % dockerfile.as_posix())
        sys.exit(1)
    opts = FFPP_DEV_CONTAINER_OPTS_DEFAULT.copy()
    opts["tag"] = f"ffpp-dev:{FFPP_VER}"
    opts["image"] = f"ffpp-dev:{FFPP_VER}"
    opts["target"] = "builder"
    client = docker.from_env()
    image, _ = client.images.build(
        path=dockerfile.parent.as_posix(),
        dockerfile=dockerfile.name,
        quiet=False,
        rm=True,
        tag=opts["tag"],
    )
    print("{} image is already built.\n".format(image.attrs["RepoTags"]))
    print("".join((bcolors.WARNING, "Remove all dangling images.", bcolors.ENDC)))
    client.images.prune(filters={"dangling": True})
    client.close()


def run_interactive():
    opts = FFPP_DEV_CONTAINER_OPTS_DEFAULT.copy()
    opts["image"] = f"ffpp-dev:{FFPP_VER}"
    print(
        bcolors.HEADER
        + "Run Docker container with image: {} interactively.".format(opts["image"])
        + bcolors.ENDC
    )
    client = docker.from_env()
    opts["name"] = "ffpp-dev-interactive"
    c_vnf = client.containers.run(**opts)
    while not c_vnf.attrs["State"]["Running"]:
        time.sleep(0.05)
        c_vnf.reload()
    # Mount BPF file system.
    # Ref: https://github.com/xdp-project/xdp-tutorial/tree/master/basic04-pinning-maps
    c_vnf.exec_run("mount -t bpf bpf /sys/fs/bpf/")
    client.close()
    subprocess.run(shlex.split("docker attach {}".format(opts["name"])), check=True)


parser = argparse.ArgumentParser(
    description="FFPP library Container development environment utility.",
    formatter_class=argparse.RawTextHelpFormatter,
)

parser.add_argument(
    "action",
    type=str,
    choices=["build_image", "run"],
    help="The action to be performed.\n"
    "\tbuild_image: Build the Docker image for FFPP development.\n"
    "\trun: Run Docker container (with ffpp-dev image) in interactive mode. The /ffpp path inside container is mounted by {} .\n".format(
        PARENT_DIR
    ),
)

parser.add_argument(
    "--experimental", action="store_true", help="Use experimental version."
)

args = parser.parse_args()

if args.experimental:
    print("*** Use experimental version!")
    FFPP_VER = "experimental"

dispatcher = {
    "build_image": build_image,
    "run": run_interactive,
}

dispatcher.get(args.action)()
