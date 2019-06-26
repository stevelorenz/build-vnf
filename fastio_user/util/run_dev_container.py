#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Run the development docker container for fastio_user library
"""

import argparse
import os
from shlex import split
from subprocess import run, PIPE

PARENT_DIR = os.path.abspath(os.path.join(os.path.curdir, os.pardir))

DOCKER_RUN_ARGS = {
    "opts": "--rm --privileged --name fastio_user -w /fastio_user",
    "dpdk_vols": "-v /sys/bus/pci/drivers:/sys/bus/pci/drivers "
    "-v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages "
    "-v /sys/devices/system/node:/sys/devices/system/node "
    "-v /dev:/dev",
    "image": "fastio_user",
    "name": "fastio_user",
    "extra_vols": "-v %s:/fastio_user" % PARENT_DIR,
}

RUN_CMD_FMT = "sudo docker run {opts} {vols} {extra_opts} {image} {cmd}"

IFACE_NAME_DPDK = "test_dpdk"
IFACE_NAME_LOADGEN = "test_loadgen"


def build_image():
    os.chdir("../")
    run_cmd = "sudo docker build -t {} --file ./Dockerfile .".format(
        DOCKER_RUN_ARGS["image"])
    run(split(run_cmd))


def build_lib():
    DOCKER_RUN_ARGS["vols"] = " ".join((DOCKER_RUN_ARGS["dpdk_vols"],
                                        DOCKER_RUN_ARGS["extra_vols"]))
    DOCKER_RUN_ARGS["cmd"] = "bash -c \"make clean && make lib \""
    DOCKER_RUN_ARGS["extra_opts"] = ""
    run_cmd = RUN_CMD_FMT.format(**DOCKER_RUN_ARGS)
    run(split(run_cmd))


def run_interactive():
    DOCKER_RUN_ARGS["vols"] = " ".join((DOCKER_RUN_ARGS["dpdk_vols"],
                                        DOCKER_RUN_ARGS["extra_vols"]))
    DOCKER_RUN_ARGS["cmd"] = "bash"
    DOCKER_RUN_ARGS["extra_opts"] = "-itd"
    run_cmd = RUN_CMD_FMT.format(**DOCKER_RUN_ARGS)
    run(split(run_cmd))
    setup_test_ifaces()
    run(split("sudo docker attach {}".format(DOCKER_RUN_ARGS["name"])))
    cleanup_test_ifaces()


def run_test():
    DOCKER_RUN_ARGS["vols"] = " ".join((DOCKER_RUN_ARGS["dpdk_vols"],
                                        DOCKER_RUN_ARGS["extra_vols"]))
    DOCKER_RUN_ARGS["cmd"] = "bash -c \"make clean && make lib && make test \""
    DOCKER_RUN_ARGS["extra_opts"] = ""
    run_cmd = RUN_CMD_FMT.format(**DOCKER_RUN_ARGS)
    run(split(run_cmd))


def run_memcheck():
    DOCKER_RUN_ARGS["vols"] = " ".join((DOCKER_RUN_ARGS["dpdk_vols"],
                                        DOCKER_RUN_ARGS["extra_vols"]))
    DOCKER_RUN_ARGS["cmd"] = "bash -c \"make clean && make lib && make mem_check \""
    DOCKER_RUN_ARGS["extra_opts"] = ""
    run_cmd = RUN_CMD_FMT.format(**DOCKER_RUN_ARGS)
    run(split(run_cmd))


def setup_test_ifaces():
    print("* Setup test ifaces")
    ret = run(r"sudo docker inspect -f '{{.State.Pid}}' %s" % DOCKER_RUN_ARGS["name"],
              check=True, shell=True, stdout=PIPE)
    pid_c = ret.stdout.decode().strip()
    cmds = [
        "sudo mkdir -p /var/run/netns",
        "sudo ln -s /proc/{}/ns/net /var/run/netns/{}".format(pid_c, pid_c),
        "sudo ip link add {} type veth peer name {}".format(
            IFACE_NAME_LOADGEN, IFACE_NAME_DPDK),
        "sudo ip link set {} up".format(IFACE_NAME_DPDK),
        "sudo ip link set {} up".format(IFACE_NAME_LOADGEN),
        "sudo ip addr add 192.168.1.17/24 dev {}".format(IFACE_NAME_LOADGEN),
        "sudo ip link set {} netns {}".format(IFACE_NAME_DPDK, pid_c),
        "sudo docker exec {} ip link set {} up".format(
            DOCKER_RUN_ARGS['name'], IFACE_NAME_DPDK)
    ]
    for c in cmds:
        run(split(c), check=True)

    cmds_shell = []


def cleanup_test_ifaces():
    print("* Cleanup test ifaces")
    cmds = [
        "sudo ip link delete {}".format(IFACE_NAME_LOADGEN)
    ]
    for c in cmds:
        run(split(c), check=False)


parser = argparse.ArgumentParser(
    description='Run development docker container.',
    formatter_class=argparse.RawTextHelpFormatter
)

parser.add_argument(
    'action', type=str,
    choices=["build_image", "build_lib", "run", "run_test", "run_memcheck"],
    help='To be performed action.\n'
    "\tbuild_image: Build docker image.\n"
    "\tbuild_lib: Build the shared library (libfastio_user.so).\n"
    "\trun: Run docker container in interactive mode.\n"
    "\trun_test: Run tests in the docker container without interaction.\n"
    "\trun_memcheck: Run memory leak checking in the docker container without interaction.\n"
)

args = parser.parse_args()

dispatcher = {
    "run": run_interactive,
    "build_lib": build_lib,
    "build_image": build_image,
    "run_test": run_test,
    "run_memcheck": run_memcheck,
}

dispatcher.get(args.action)()
