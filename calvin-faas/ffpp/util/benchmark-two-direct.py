#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Minimal benchmark setup: Two containers connected directly via a veth pair.
"""

import argparse
import os
import sys
import time

from shlex import split
from subprocess import run, PIPE

import docker

with open("../VERSION", "r") as vfile:
    FFPP_VER = vfile.read().strip()

FFPP_DEV_CONTAINER_OPTS_DEFAULT = {
    # I know --privileged is a bad/danger option. It is just used for tests.
    "auto_remove": True,
    "detach": True,  # -d
    "init": True,
    "privileged": True,
    "tty": True,  # -t
    "stdin_open": True,  # -i
    "volumes": {
        "/sys/bus/pci/drivers": {"bind": "/sys/bus/pci/drivers", "mode": "rw"},
        "/sys/kernel/mm/hugepages": {"bind": "/sys/kernel/mm/hugepages", "mode": "rw"},
        "/sys/devices/system/node": {"bind": "/sys/devices/system/node", "mode": "rw"},
        "/dev": {"bind": "/dev", "mode": "rw"},
    },
    "working_dir": "/ffpp",
    "image": "ffpp-dev:%s" % (FFPP_VER),
    "command": "bash",
    "labels": {"group": "ffpp-dev-benchmark"},
}


def setup_two_direct_veth(pktgen_image):
    client = docker.from_env()
    vnf_args = FFPP_DEV_CONTAINER_OPTS_DEFAULT.copy()
    vnf_args["name"] = "vnf"
    c_vnf = client.containers.run(**vnf_args)

    while not c_vnf.attrs["State"]["Running"]:
        time.sleep(0.05)
        c_vnf.reload()
    c_vnf_pid = c_vnf.attrs["State"]["Pid"]

    pktgen_args = FFPP_DEV_CONTAINER_OPTS_DEFAULT.copy()
    pktgen_args["name"] = "pktgen"
    c_pktgen = client.containers.run(**pktgen_args)
    while not c_pktgen.attrs["State"]["Running"]:
        time.sleep(0.05)
        c_pktgen.reload()
    c_pktgen_pid = c_pktgen.attrs["State"]["Pid"]
    print(
        "* Both VNF and Pktgen containers are running with PID: {},{}".format(
            c_vnf_pid, c_pktgen_pid
        )
    )

    print("* Connect ffpp-dev-vnf and pktgen container with veth pairs.")
    cmds = [
        "mkdir -p /var/run/netns",
        "ln -s /proc/{}/ns/net /var/run/netns/{}".format(c_vnf_pid, c_vnf_pid),
        "ln -s /proc/{}/ns/net /var/run/netns/{}".format(c_pktgen_pid, c_pktgen_pid),
        "ip link add vnf type veth peer name pktgen",
        "ip link set vnf up",
        "ip link set pktgen up",
        "ip link set vnf netns {}".format(c_vnf_pid),
        "ip link set pktgen netns {}".format(c_pktgen_pid),
        "docker exec vnf ip link set vnf up",
        "docker exec pktgen ip link set pktgen up",
        "docker exec vnf ip addr add 192.168.17.1/24 dev vnf",
        "docker exec pktgen ip addr add 192.168.17.2/24 dev pktgen",
    ]
    for c in cmds:
        run(split(c), check=True)

    print(
        "* Setup finished. Run 'docker attach ffpp-dev-vnf' (or pktgen) to attach to the running containers."
    )


def teardown():
    client = docker.from_env()
    c_list = client.containers.list(
        all=True, filters={"label": "group=ffpp-dev-benchmark"}
    )
    for c in c_list:
        print("* Remove container: {}".format(c.name))
        c.remove(force=True)
    client.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Minimal benchmark setup: Two containers connected directly via a veth pair."
    )
    parser.add_argument(
        "action",
        type=str,
        choices=["setup", "teardown"],
        help="The action to perform.",
    )
    parser.add_argument(
        "--pktgen_image",
        type=str,
        default=FFPP_DEV_CONTAINER_OPTS_DEFAULT["image"],
        help="The Docker image to create the pktgen.",
    )

    args = parser.parse_args()
    if args.action == "setup":
        setup_two_direct_veth(args.pktgen_image)
    elif args.action == "teardown":
        teardown()
    else:
        pass
