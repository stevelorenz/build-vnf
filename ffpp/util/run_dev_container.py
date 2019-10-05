#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Run the development docker container for FFPP library
"""

import argparse
import os
from shlex import split
from subprocess import run, PIPE

PARENT_DIR = os.path.abspath(os.path.join(os.path.curdir, os.pardir))

DOCKER_RUN_ARGS = {
    # essential options
    "opts": "--rm --privileged -w /ffpp",
    "dpdk_vols": "-v /sys/bus/pci/drivers:/sys/bus/pci/drivers "
    "-v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages "
    "-v /sys/devices/system/node:/sys/devices/system/node "
    "-v /dev:/dev",
    "image": "ffpp",
    "ver": "latest",
    "name": "ffpp",
    "extra_vols": "-v %s:/ffpp" % PARENT_DIR,
}

RUN_CMD_FMT = "docker run {opts} {vols} {extra_opts} {image} {cmd}"

IFACE_NAME_DPDK_IN = "test_dpdk_in"
IFACE_NAME_LOADGEN_IN = "test_loadgen_in"


def build_image():
    os.chdir("../")
    run_cmd = "docker build --compress --rm -t {}:{} --file ./Dockerfile .".format(
        DOCKER_RUN_ARGS["image"], DOCKER_RUN_ARGS["ver"]
    )
    run(split(run_cmd))


def build_lib():
    DOCKER_RUN_ARGS["vols"] = " ".join(
        (DOCKER_RUN_ARGS["dpdk_vols"], DOCKER_RUN_ARGS["extra_vols"])
    )
    DOCKER_RUN_ARGS["cmd"] = 'bash -c "make clean && make lib "'
    DOCKER_RUN_ARGS["extra_opts"] = ""
    run_cmd = RUN_CMD_FMT.format(**DOCKER_RUN_ARGS)
    run(split(run_cmd))


def build_lib_debug():
    DOCKER_RUN_ARGS["vols"] = " ".join(
        (DOCKER_RUN_ARGS["dpdk_vols"], DOCKER_RUN_ARGS["extra_vols"])
    )
    DOCKER_RUN_ARGS["cmd"] = 'bash -c "make clean && make lib_debug "'
    DOCKER_RUN_ARGS["extra_opts"] = ""
    run_cmd = RUN_CMD_FMT.format(**DOCKER_RUN_ARGS)
    run(split(run_cmd))


def run_interactive():
    # Avoid conflict with other non-interactive actions
    cname = "ffpp_interactive"
    DOCKER_RUN_ARGS["vols"] = " ".join(
        (DOCKER_RUN_ARGS["dpdk_vols"], DOCKER_RUN_ARGS["extra_vols"])
    )
    DOCKER_RUN_ARGS["cmd"] = "bash"
    DOCKER_RUN_ARGS["extra_opts"] = "-itd --name %s" % cname
    run_cmd = RUN_CMD_FMT.format(**DOCKER_RUN_ARGS)
    run(split(run_cmd))
    setup_test_ifaces(cname)
    run(split("docker attach {}".format(cname)))
    cleanup_test_ifaces()


def run_test():
    DOCKER_RUN_ARGS["vols"] = " ".join(
        (DOCKER_RUN_ARGS["dpdk_vols"], DOCKER_RUN_ARGS["extra_vols"])
    )
    DOCKER_RUN_ARGS["cmd"] = 'bash -c "make clean && make lib_debug && make test "'
    DOCKER_RUN_ARGS["extra_opts"] = ""
    run_cmd = RUN_CMD_FMT.format(**DOCKER_RUN_ARGS)
    run(split(run_cmd))


def run_memcheck():
    DOCKER_RUN_ARGS["vols"] = " ".join(
        (DOCKER_RUN_ARGS["dpdk_vols"], DOCKER_RUN_ARGS["extra_vols"])
    )
    DOCKER_RUN_ARGS["cmd"] = 'bash -c "make clean && make lib_debug && make mem_check "'
    DOCKER_RUN_ARGS["extra_opts"] = ""
    run_cmd = RUN_CMD_FMT.format(**DOCKER_RUN_ARGS)
    run(split(run_cmd))


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
    description="Run development docker container.",
    formatter_class=argparse.RawTextHelpFormatter,
)

parser.add_argument(
    "action",
    type=str,
    choices=[
        "build_image",
        "build_lib",
        "build_lib_debug",
        "run",
        "run_test",
        "run_memcheck",
    ],
    help="To be performed action.\n"
    "\tbuild_image: Build docker image.\n"
    "\tbuild_lib: Build the shared library (libffpp.so).\n"
    "\tbuild_lib_debug: Build the shared library in debug mode(-g and -O0).\n"
    "\trun: Run docker container in interactive mode.\n"
    "\trun_test: Run tests in the docker container without interaction.\n"
    "\trun_memcheck: Run memory leak checking in the docker container without interaction.\n",
)

args = parser.parse_args()

dispatcher = {
    "run": run_interactive,
    "build_lib": build_lib,
    "build_lib_debug": build_lib_debug,
    "build_image": build_image,
    "run_test": run_test,
    "run_memcheck": run_memcheck,
}

dispatcher.get(args.action)()
