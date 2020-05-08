#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: FFPP library Container development environment utility.
"""

import argparse
import os
import sys

from pathlib import Path
from shlex import split
from subprocess import run, PIPE

FFPP_VER = "0.0.1"

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


DOCKER_RUN_ARGS = {
    # Essential options
    # I know --privileged is a bad/danger option. It is just used for tests.
    "opts": "--rm --privileged -w /ffpp",
    "dpdk_vols": "-v /sys/bus/pci/drivers:/sys/bus/pci/drivers "
    "-v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages "
    "-v /sys/devices/system/node:/sys/devices/system/node "
    "-v /dev:/dev",
    "image": "ffpp-dev",
    "ver": FFPP_VER,
    "name": "ffpp",
    "extra_vols": "-v %s:/ffpp" % PARENT_DIR,
}

RUN_CMD_FMT = "docker run {opts} {vols} {extra_opts} {image}:{ver} {cmd}"

IFACE_NAME_DPDK_IN = "test_dpdk_in"
IFACE_NAME_LOADGEN_IN = "test_loadgen_in"


def build_image():
    print(
        bcolors.HEADER
        + "ACTION: Build docker image for FFPP development."
        + bcolors.ENDC
    )
    dockerfile = Path("../Dockerfile")
    if not dockerfile.is_file():
        print("Can not find the Dockerfile in path: %s." % dockerfile.as_posix())
        sys.exit(1)
    os.chdir("../")
    run_cmd = "docker build --compress --rm -t {}:{} --file ./Dockerfile .".format(
        DOCKER_RUN_ARGS["image"], DOCKER_RUN_ARGS["ver"]
    )
    run(split(run_cmd))
    clean_cmd = "docker image prune -f"
    run(split(clean_cmd))


def run_interactive():
    # Avoid conflict with other non-interactive actions
    print(
        bcolors.HEADER
        + "ACTION: Run Docker image: %s:%s interactivly."
        % (DOCKER_RUN_ARGS["image"], DOCKER_RUN_ARGS["ver"])
        + bcolors.ENDC
    )
    print(
        bcolors.WARNING + "- This container will be removed after exit" + bcolors.ENDC
    )
    cname = "ffpp_interactive"
    DOCKER_RUN_ARGS["vols"] = DOCKER_RUN_ARGS["dpdk_vols"]
    # DOCKER_RUN_ARGS["vols"] = " ".join(
    #     (DOCKER_RUN_ARGS["dpdk_vols"], DOCKER_RUN_ARGS["extra_vols"])
    # )
    DOCKER_RUN_ARGS["cmd"] = "bash"
    DOCKER_RUN_ARGS["extra_opts"] = "-itd --name %s" % cname
    run_cmd = RUN_CMD_FMT.format(**DOCKER_RUN_ARGS)
    run(split(run_cmd))
    run(split("docker attach {}".format(cname)))


def run_interactive_with_ffpp_path_mount():
    # Avoid conflict with other non-interactive actions
    print(
        bcolors.HEADER
        + """ACTION: Run Docker image: %s:%s interactivly. The /ffpp path inside container is mounted by ../ path on the host."""
        % (DOCKER_RUN_ARGS["image"], DOCKER_RUN_ARGS["ver"])
        + bcolors.ENDC
    )
    print(
        bcolors.WARNING + "- This container will be removed after exit" + bcolors.ENDC
    )
    print(
        bcolors.WARNING
        + """- The /ffpp path inside the container is mounted by ../ on the host. So any changes in /ffpp is shared between container and host. It is convinient for dev and test."""
        + bcolors.ENDC
    )
    cname = "ffpp_interactive"
    DOCKER_RUN_ARGS["vols"] = " ".join(
        (DOCKER_RUN_ARGS["dpdk_vols"], DOCKER_RUN_ARGS["extra_vols"])
    )
    DOCKER_RUN_ARGS["cmd"] = "bash"
    DOCKER_RUN_ARGS["extra_opts"] = "-itd --name %s" % cname
    run_cmd = RUN_CMD_FMT.format(**DOCKER_RUN_ARGS)
    run(split(run_cmd))
    run(split("docker attach {}".format(cname)))


def setup_test_ifaces(cname):
    print("* Setup test ifaces")
    ret = run(
        r"docker inspect -f '{{.State.Pid}}' %s" % cname,
        check=True,
        shell=True,
        stdout=PIPE,
    )
    pid_c = ret.stdout.decode().strip()
    cmds = [
        "mkdir -p /var/run/netns",
        "ln -s /proc/{}/ns/net /var/run/netns/{}".format(pid_c, pid_c),
        "ip link add {} type veth peer name {}".format(
            IFACE_NAME_LOADGEN_IN, IFACE_NAME_DPDK_IN
        ),
        "ip link set {} up".format(IFACE_NAME_DPDK_IN),
        "ip link set {} up".format(IFACE_NAME_LOADGEN_IN),
        "ip addr add 192.168.1.17/24 dev {}".format(IFACE_NAME_LOADGEN_IN),
        "ip link set {} netns {}".format(IFACE_NAME_DPDK_IN, pid_c),
        "docker exec {} ip link set {} up".format(cname, IFACE_NAME_DPDK_IN),
    ]
    for c in cmds:
        run(split(c), check=True)


def cleanup_test_ifaces():
    print("* Cleanup test ifaces")
    cmds = ["ip link delete {}".format(IFACE_NAME_LOADGEN_IN)]
    for c in cmds:
        run(split(c), check=False)


parser = argparse.ArgumentParser(
    description="FFPP library Container development environment utility.",
    formatter_class=argparse.RawTextHelpFormatter,
)

parser.add_argument(
    "action",
    type=str,
    choices=["build_image", "run", "run_mount"],
    help="The action to be performed.\n"
    "\tbuild_image: Build the Docker image for FFPP development.\n"
    "\trun: Run Docker container (with ffpp-dev image) in interactive mode.\n"
    "\trun_mount: Like run command. The /ffpp path inside container is mounted by ../ .\n",
)

args = parser.parse_args()

dispatcher = {
    "build_image": build_image,
    "run": run_interactive,
    "run_mount": run_interactive_with_ffpp_path_mount,
}

dispatcher.get(args.action)()
