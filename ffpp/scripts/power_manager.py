#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Run/stop power manager Docker container on the host OS.
"""

import argparse
import os
import shlex
import subprocess
import sys
import time

import docker

with open("../VERSION", "r") as vfile:
    FFPP_VER = vfile.read().strip()

PARENT_DIR = os.path.abspath(os.path.join(os.path.curdir, os.pardir))
BPF_MAP_BASEDIR = "/sys/fs/bpf"
LOCKED_IN_MEMORY_SIZE = -1  # -> unlimited

POWER_MANAGER_OPTS_DEFAULT = {
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
        "/sys/devices/system/cpu": {"bind": "/sys/devices/system/cpu", "mode": "rw"},
        "/dev": {"bind": "/dev", "mode": "rw"},
        "/var/run/docker.sock": {"bind": "/var/run/docker.sock", "mode": "rw"},
        PARENT_DIR: {"bind": "/ffpp", "mode": "rw"},
        BPF_MAP_BASEDIR: {"bind": BPF_MAP_BASEDIR, "mode": "rw"},
    },
    "working_dir": "/ffpp",
    "image": "ffpp:%s" % (FFPP_VER),
    "command": "bash",
    "labels": {"group": "ffpp-power-manager"},
    # Ulimits, memlock should be enough to load eBPF maps.
    "ulimits": [
        docker.types.Ulimit(
            name="memlock", hard=LOCKED_IN_MEMORY_SIZE, soft=LOCKED_IN_MEMORY_SIZE
        )
    ],
}


def start(core):
    client = docker.from_env()

    pm_args = POWER_MANAGER_OPTS_DEFAULT.copy()
    pm_args["name"] = "power-manager"
    # PM need host network mode to see all interfaces
    pm_args["network_mode"] = "host"
    # One core is sufficient for the PM
    pm_args["cpuset_cpus"] = core
    c_pm = client.containers.run(**pm_args)

    while not c_pm.attrs["State"]["Running"]:
        time.sleep(0.05)
        c_pm.reload()

    c_pm_pid = c_pm.attrs["State"]["Pid"]
    print("* The power manager container is running with PID: {}".format(c_pm_pid))

    client.close()


def stop():
    client = docker.from_env()
    c_list = client.containers.list(
        all=True, filters={"label": "group=ffpp-power-manager"}
    )
    for c in c_list:
        print("* Remove container: {}".format(c.name))
        c.remove(force=True)
    client.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Run/stop power manager Docker container on the host OS."
    )
    parser.add_argument(
        "action", type=str, choices=["run", "stop"], help="The action to perform."
    )
    # Can be any core on the system, best a different from the CNF ones
    parser.add_argument(
        "-c",
        default="5",
        const="5",
        nargs="?",
        type=str,
        help="Core the power-manager runs on.",
    )

    args = parser.parse_args()

    if args.action == "run":
        start(args.c)
    elif args.action == "stop":
        stop(args.i)
