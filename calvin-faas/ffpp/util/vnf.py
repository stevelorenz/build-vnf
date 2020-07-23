#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Setup VNF
"""

import argparse
import os
import shutil
import sys
import time
import multiprocessing

from shlex import split
from subprocess import run, PIPE

import docker

with open ("../VERSION", "r") as vfile:
    FFPP_VER = vfile.read().strip()

PARENT_DIR = os.path.abspath(os.path.join(os.path.curdir, os.pardir))

FFPP_DEV_CONTAINER_OPTS_DEFAULT = {
    "auto_remove": True,
    "cap_add": "ALL",
    "detach": True,  # -d
    "init": True,
    # I know --privileged is a bad/danger option. It is just used for tests.
    "privileged": True,
    "tty": True,  # -t
    "stdin_open": True,  # -i
    "volumes": {
        "/sys/bus/pci/drivers": {"bind": "/sys/bus/pci/drivers", "mode": "rw"},
        "/sys/bus/pci/devices": {"bind": "/sys/bus/pci/devices", "mode": "rw"},
        "/sys/kernel/mm/hugepages": {"bind": "/sys/kernel/mm/hugepages", "mode": "rw"},
        "/sys/devices/system/node": {"bind": "/sys/devices/system/node", "mode": "rw"},
        "/lib/modules": {"bind": "/lib/modules", "mode": "rw"},
        "/mnt/huge": {"bind": "/mnt/huge", "mode": "rw"},
        "/dev": {"bind": "/dev", "mode": "rw"},
        PARENT_DIR: {"bind": "/ffpp", "mode": "rw"},
    },
    "working_dir": "/ffpp",
    "image": "ffpp-dev:%s" % (FFPP_VER),
    "command": "bash",
    "labels": {"group": "ffpp-vnf"},
    "nano_cpus": int(1e9),
    "cpuset_cpus": "1,3",
}


def run():
    client = docker.from_env()

    pktgen_args = FFPP_DEV_CONTAINER_OPTS_DEFAULT.copy()
    pktgen_args["name"] = "vnf"
    pktgen_args["network_mode"] = "host"
    c_pktgen = client.containers.run(**pktgen_args)

    while not c_pktgen.attrs["State"]["Running"]:
        time.sleep(0.05)
        c_pktgen.reload()

    c_pktgen_pid = c_pktgen.attrs["State"]["Pid"]
    c_pktgen.exec_run("mount -t bpf bpf /sys/fs/bpf")
    print("* The VNF container is running with PID: ", c_pktgen_pid)

    client.close()


def stop():
    client = docker.from_env()
    c_list = client.containers.list(
        all=True, filters={"label": "group=ffpp-vnf"}
    )
    for c in c_list:
        print("Remove container: ", c.name)
        c.remove(force=True)
    client.close()
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Run/ Stop Trex pktgen container on the host OS"
    )
    parser.add_argument(
        "action", type=str, choices=["run", "stop"], help="The action to perform"
    )

    args = parser.parse_args()

    if args.action == "run":
        run()
    elif args.action == "stop":
        stop()
