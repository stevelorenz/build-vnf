#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Setup VNF

ToDo: soft-code physical interface and its MAC addresses
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

with open("../VERSION", "r") as vfile:
    FFPP_VER = vfile.read().strip()

PARENT_DIR = os.path.abspath(os.path.join(os.path.curdir, os.pardir))
BPF_MAP_BASEDIR = "/sys/fs/bpf"
LOCKED_IN_MEMORY_SIZE = -1  # -> unlimited

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
        PARENT_DIR: {"bind": "/ffpp", "mode": "rw"},
    },
    "working_dir": "/ffpp/user/related_works/l2fwd_power/",
    "image": "ffpp-dev:%s" % (FFPP_VER),
    "command": "bash",
    "labels": {"group": "ffpp-vnf"},
    "cpuset_cpus": "1,3,5,7",
    "ulimits": [
        docker.types.Ulimit(
            name="memlock", hard=LOCKED_IN_MEMORY_SIZE, soft=LOCKED_IN_MEMORY_SIZE
        )
    ],
}


def start(num_vnf, host_nw, load_pm, load_fb):
    client = docker.from_env()

    vnf_args = FFPP_DEV_CONTAINER_OPTS_DEFAULT.copy()
    c_vnf_pid = list()
    for m_vnf in range(0, num_vnf, 1):
        vnf_args["name"] = "vnf-{}".format(m_vnf)
        if not host_nw:
            vnf_args["network_mode"] = "host"
        c_vnf = client.containers.run(**vnf_args)

        while not c_vnf.attrs["State"]["Running"]:
            time.sleep(0.05)
            c_vnf.reload()

        c_vnf_pid.append(c_vnf.attrs["State"]["Pid"])
        c_vnf.exec_run("mount -t bpf bpf /sys/fs/bpf")
        print("* The VNF container is running with PID: ", c_vnf_pid[m_vnf])

        # Setup veth peers within the containers and
        # load all neccessary XDP programs
        if host_nw:
            setup_host_network(c_vnf_pid[m_vnf], m_vnf, load_pm, load_fb, num_vnf)

    client.close()

    print("* Current XDP status: ")
    run(split("xdp-loader status"), check=True)
    print("* Setup finished")


def setup_host_network(c_vnf_pid, m_vnf, load_pm, load_fb, num_vnf):
    os.chdir(PARENT_DIR)
    # Setup Veth peers
    cmds = [
        "mkdir -p /var/run/netns",
        "ln -s /proc/{}/ns/net /var/run/netns/{}".format(c_vnf_pid, c_vnf_pid),
        "ip link add vnf-in type veth peer name vnf{}-in-root".format(m_vnf),
        "ip link set vnf{}-in-root up".format(m_vnf),
        "ip link set vnf-in netns {}".format(c_vnf_pid),
        "docker exec vnf-{} ip link set vnf-in up".format(m_vnf),
        "ip link add vnf-out type veth peer name vnf{}-out-root".format(m_vnf),
        "ip link set vnf{}-out-root up".format(m_vnf),
        "ip link set vnf-out netns {}".format(c_vnf_pid),
        "docker exec vnf-{} ip link set vnf-out up".format(m_vnf),
    ]
    for c in cmds:
        run(split(c), check=False)

    client = docker.from_env()
    c_vnf = client.containers.list(
        all=True, filters={"label": "group=ffpp-vnf", "name": "vnf-{}".format(m_vnf)}
    )[0]

    # Load XDP pass on vnf-in and vnf-out for af_packet
    print("* Load XDP-Pass on ingress and egress interface inside VNF")
    exit_code, out = c_vnf.exec_run(
        cmd="make",
        workdir="/ffpp/kern/xdp_time",
    )
    if exit_code != 0:
        print("ERR: Failed to compile xdp-time program in vnf.")
        sys.exit(1)
    for iface in ["vnf-in", "vnf-out"]:
        # Load traffic monitoring on the vnf-in interface
        # If we do not need it, it's unloaded anyway by af_xdp
        exit_code, out = c_vnf.exec_run(
            cmd="./xdp_time_loader {}".format(iface),
            workdir="/ffpp/kern/xdp_time/",
        )
        if exit_code != 0:
            print("ERR: Failed to load xdp-time program on interface {}".format(iface))
            sys.exit(1)

    # Load XDP forwarder between phy NIC and virt vnf-in-root/ vnf-out-root
    # We currently use the xdp_stats_map from the xdp_fwd program
    with open("/sys/class/net/vnf{}-in-root/address".format(m_vnf), "r") as f:
        vnf_in_root_mac = f.read().strip()
    with open("/sys/class/net/vnf{}-out-root/address".format(m_vnf), "r") as f:
        vnf_out_root_mac = f.read().strip()
    _, out = c_vnf.exec_run(cmd="cat /sys/class/net/vnf-in/address")
    vnf_in_mac = out.decode("ascii").strip()
    _, out = c_vnf.exec_run(cmd="cat /sys/class/net/vnf-out/address")
    vnf_out_mac = out.decode("ascii").strip()
    pktgen_out_phy_mac = "0c:42:a1:51:41:d8"  # src enp65s0f0
    pktgen_in_phy_mac = "0c:42:a1:51:41:d9"  # dst enp65s0f1
    vnf_in_phy_mac = "0c:42:a1:51:42:bc"  # enp5s0f0
    vnf_out_phy_mac = "0c:42:a1:51:42:bd"  # enp5s0f1
    # To differentiate flows ;)
    upd_payload_size = [1400, 1399]

    print("- The MAC address of vnf-in in vnf-{}: {}".format(m_vnf, vnf_in_mac))
    print("- The MAC address of vnf-out in vnf-{}: {}".format(m_vnf, vnf_out_mac))
    print("- The MAC address of vnf{}-in-root: {}".format(m_vnf, vnf_in_root_mac))
    print("- The MAC address of vnf{}-out-root: {}".format(m_vnf, vnf_out_root_mac))

    print("* Run xdp_fwd programs between physical and virtual interface")

    # Decide which forward programs to use
    if num_vnf == 1:
        xdp_fwd_dir = os.path.abspath("./kern/xdp_fwd/")
    else:
        xdp_fwd_dir = os.path.abspath("./kern/xdp_fwd_two_vnf/")
    print("++ path: ", xdp_fwd_dir)

    if not os.path.exists(xdp_fwd_dir):
        print("ERR: Can not find the xdp_fwd program.")
        sys.exit(1)
    os.chdir(xdp_fwd_dir)
    # @load_pm load setup for util power management
    # @load_fb load setup for feedback power managemt
    if load_pm and not load_fb:
        if not os.path.exists(
            os.path.join(xdp_fwd_dir, "./xdp_fwd_time_kern.o")
        ) and not os.path.exists(os.path.join(xdp_fwd_dir, "./xdp_fwd_kern.o")):
            print("\tINFO: Compile xdp_fwd_time program")
            run(split("make"), check=True)
        print("\t- Load xdp_fwd_time kernel programs.")
        if m_vnf == 0:  # only once on physical interface
            run(split("sudo ./xdp_fwd_loader enp5s0f0 xdp_fwd_time_kern.o"), check=True)
        run(split("sudo ./xdp_fwd_loader vnf{}-out-root".format(m_vnf)), check=True)
    elif load_fb:
        if not os.path.exists(os.path.join(xdp_fwd_dir, "./xdp_fwd_fb_kern.o")):
            print("\tINFO: Compile xdp_fwd_fb prgram")
            run(split("make"), check=True)
        print("\t- Load xdp_fwd_fb kernel programs.")
        if m_vnf == 0:
            run(split("sudo ./xdp_fwd_loader enp5s0f0 xdp_fwd_fb_kern.o"), check=True)
        run(
            split(
                "sudo ./xdp_fwd_loader vnf{}-out-root xdp_fwd_fb_kern.o".format(m_vnf)
            ),
            check=True,
        )
    # If no PM is wanted
    else:
        if not os.path.exists(os.path.join(xdp_fwd_dir, "./xdp_fwd_kern.o")):
            print("\tINFO: Compile xdp_fwd program")
            run(split("make"), check=True)
        print("\t- Load xdp_fwd kernel programs.")
        if m_vnf == 0:
            run(split("sudo ./xdp_fwd_loader enp5s0f0"), check=True)
        run(split("sudo ./xdp_fwd_loader vnf{}-out-root".format(m_vnf)), check=True)

    print("\t- Add forwarding between physical and virtual interface")
    run(
        split(
            "./xdp_fwd_user -i enp5s0f0 -r vnf{}-in-root -s {} -d {} -w {} -p {}".format(
                m_vnf,
                pktgen_out_phy_mac,
                vnf_in_mac,
                vnf_in_root_mac,
                upd_payload_size[m_vnf],
            )
        )
    )
    # The L2FWD doesn't do MAC updating
    run(
        split(
            "./xdp_fwd_user -i vnf{}-out-root -r enp5s0f1 -s {} -d {} -w {} -p {}".format(
                m_vnf,
                vnf_out_mac,
                pktgen_in_phy_mac,
                vnf_out_phy_mac,
                upd_payload_size[m_vnf],
            )
        )
    )

    client.close()


def unloadXDP(num_vnf):
    ifaces = list()
    # Get list of all loaded XDP programs
    for iface in range(0, num_vnf, 1):
        ifaces.append("vnf{}-out-root".format(iface))
    ifaces.append("enp5s0f0")
    for iface in ifaces:
        bpf_map_dir = os.path.join(BPF_MAP_BASEDIR, iface)
        if os.path.exists(bpf_map_dir):
            print("- Remove BPF maps directory: {}".format(bpf_map_dir))
            shutil.rmtree(bpf_map_dir)
        run(split("sudo xdp-loader unload {}".format(iface)), check=True)
    print("* Unloaded XPD programm from interface ")


def stop(num_vnf, host_nw):
    if host_nw:
        unloadXDP(num_vnf)
    client = docker.from_env()
    c_list = client.containers.list(all=True, filters={"label": "group=ffpp-vnf"})
    for c in c_list:
        print("- Remove container: ", c.name)
        c.remove(force=True)
    client.close()
    print("* Setup removed")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Run/ Stop vnf container on the host OS"
    )
    parser.add_argument(
        "action", type=str, choices=["run", "stop"], help="The action to perform"
    )
    parser.add_argument(
        "--no_host_network",
        type=bool,
        nargs="?",
        default=False,
        const=True,
        help="Wether to setup the Veth peers or not; default is to setup",
    )
    parser.add_argument(
        "--no_pm",
        type=bool,
        nargs="?",
        default=False,
        const=True,
        help="Load the xdp forwarder without traffic monitoring",
    )
    parser.add_argument(
        "--feedback",
        type=bool,
        nargs="?",
        default=False,
        const=True,
        help="Load traffic monitor on egress interface to obtain feedback",
    )
    parser.add_argument(
        "--num_vnf",
        type=int,
        default=1,
        help="Number of VNF containers to deploy",
    )

    args = parser.parse_args()

    if args.action == "run":
        start(args.num_vnf, not (args.no_host_network), not (args.no_pm), args.feedback)
    elif args.action == "stop":
        stop(args.num_vnf, not (args.no_host_network))
