#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
A script to measure the RW latencies of eBPF/XDP maps.
"""

import argparse
import csv
import os
import pdb
import shlex
import subprocess
import sys

import pyroute2
from pyroute2 import IPRoute
from pyroute2 import netns

import docker


XDP_TIME_DIR = "/vagrant/ffpp/kern/xdp_time/"


def run_tests(veth_num: int):
    client = docker.from_env()
    tester = client.containers.run(
        "ffpp-dev:0.2.0",
        name="tester",
        init=True,
        tty=True,
        detach=True,
        command="bash",
        privileged=True,
        network_mode="host",
        volumes={
            "/sys/fs/bpf": {"bind": "/sys/fs/bpf", "mode": "rw"},
            os.path.abspath(os.path.curdir): {"bind": "/test", "mode": "rw"},
        },
        working_dir="/test",
    )

    if not os.path.exists(XDP_TIME_DIR):
        print("Can not find the path of the xdp_time program.", file=sys.stderr)
        return
    else:
        os.chdir(XDP_TIME_DIR)

    print(f"* Create {veth_num} veth pairs...")
    with IPRoute() as ipr:

        for i in range(veth_num):
            netns.create(f"test{i}")
            # create interface pair
            ipr.link("add", ifname=f"r{i}", kind="veth", peer=f"v{i}")
            idx = ipr.link_lookup(ifname=f"v{i}")[0]
            ipr.link("set", index=idx, net_ns_fd=f"test{i}")
            subprocess.run(shlex.split(f"sudo ./xdp_time_loader r{i}"), check=True)

        pdb.set_trace()

        for i in range(veth_num):
            netns.remove(f"test{i}")

    tester.remove(force=True)
    client.close()


if __name__ == "__main__":
    # Make sure xterm works inside sudo env.
    sudo_user = os.environ.get("SUDO_USER", None)
    if not sudo_user:
        print("Error: Run this script with sudo.")
        sys.exit(1)

    # TODO: Add this pre-setup in comnetsemu.
    if os.path.exists(f"/home/{sudo_user}/.Xauthority"):
        subprocess.run(
            f"xauth add $(xauth -f /home/{sudo_user}/.Xauthority list|tail -1)",
            check=True,
            shell=True,
        )

    parser = argparse.ArgumentParser(
        description="A script to measure the RW latencies of eBPF/XDP maps."
    )
    parser.add_argument(
        "--veth_num", type=int, default=3, help="Number of veth devices."
    )
    args = parser.parse_args()

    print(f"The number of veth pairs: {args.veth_num}.")

    run_tests(args.veth_num)

    print(f"Measurements finished!")
