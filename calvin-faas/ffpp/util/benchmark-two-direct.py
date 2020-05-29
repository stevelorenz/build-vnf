#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Minimal benchmark setup: Two containers connected directly via a veth pair.
"""

import argparse
import os
import shutil
import sys
import time

from shlex import split
from subprocess import run, PIPE

import docker

with open("../VERSION", "r") as vfile:
    FFPP_VER = vfile.read().strip()

PARENT_DIR = os.path.abspath(os.path.join(os.path.curdir, os.pardir))
BPF_MAP_BASEDIR = "/sys/fs/bpf"

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
        PARENT_DIR: {"bind": "/ffpp", "mode": "rw"},
    },
    "working_dir": "/ffpp",
    "image": "ffpp-dev:%s" % (FFPP_VER),
    "command": "bash",
    "labels": {"group": "ffpp-dev-benchmark"},
    "nano_cpus": int(5e8),  # Avoid 100% CPU usage.
}


def setup_common(pktgen_image):
    client = docker.from_env()
    vnf_args = FFPP_DEV_CONTAINER_OPTS_DEFAULT.copy()
    vnf_args["name"] = "vnf"
    c_vnf = client.containers.run(**vnf_args)

    while not c_vnf.attrs["State"]["Running"]:
        time.sleep(0.05)
        c_vnf.reload()
    c_vnf_pid = c_vnf.attrs["State"]["Pid"]
    c_vnf.exec_run("mount -t bpf bpf /sys/fs/bpf/")

    pktgen_args = FFPP_DEV_CONTAINER_OPTS_DEFAULT.copy()
    pktgen_args["name"] = "pktgen"
    pktgen_args["image"] = pktgen_image
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
    client.close()
    return (c_vnf_pid, c_pktgen_pid)


def setup_two_direct_veth(pktgen_image):
    c_vnf_pid, c_pktgen_pid = setup_common(pktgen_image)
    print("* Connect ffpp-dev-vnf and pktgen container with veth pairs.")
    cmds = [
        "mkdir -p /var/run/netns",
        "ln -s /proc/{}/ns/net /var/run/netns/{}".format(c_vnf_pid, c_vnf_pid),
        "ln -s /proc/{}/ns/net /var/run/netns/{}".format(c_pktgen_pid, c_pktgen_pid),
        "ip link add vnf-in type veth peer name pktgen-out",
        "ip link set vnf-in up",
        "ip link set pktgen-out up",
        "ip link set vnf-in netns {}".format(c_vnf_pid),
        "ip link set pktgen-out netns {}".format(c_pktgen_pid),
        "docker exec vnf ip link set vnf-in up",
        "docker exec pktgen ip link set pktgen-out up",
        "docker exec vnf ip addr add 192.168.17.1/24 dev vnf-in",
        "docker exec pktgen ip addr add 192.168.17.2/24 dev pktgen-out",
        "ip link add vnf-out type veth peer name pktgen-in",
        "ip link set vnf-out up",
        "ip link set pktgen-in up",
        "ip link set vnf-out netns {}".format(c_vnf_pid),
        "ip link set pktgen-in netns {}".format(c_pktgen_pid),
        "docker exec vnf ip link set vnf-out up",
        "docker exec pktgen ip link set pktgen-in up",
        "docker exec vnf ip addr add 192.168.18.1/24 dev vnf-out",
        "docker exec pktgen ip addr add 192.168.18.2/24 dev pktgen-in",
        "docker exec pktgen ping -q -c 2 192.168.17.1",
        "docker exec pktgen ping -q -c 2 192.168.18.1",
    ]
    for c in cmds:
        run(split(c), check=True)

    print(
        "* Setup finished. Run 'docker attach ffpp-dev-vnf' (or pktgen) to attach to the running containers."
    )


def setup_two_veth_xdp_fwd(pktgen_image):
    c_vnf_pid, c_pktgen_pid = setup_common(pktgen_image)
    cmds = [
        "mkdir -p /var/run/netns",
        "ln -s /proc/{}/ns/net /var/run/netns/{}".format(c_vnf_pid, c_vnf_pid),
        "ln -s /proc/{}/ns/net /var/run/netns/{}".format(c_pktgen_pid, c_pktgen_pid),
        "ip link add vnf-in type veth peer name vnf-in-root",
        "ip link set vnf-in-root up",
        "ip link set vnf-in netns {}".format(c_vnf_pid),
        "docker exec vnf ip link set vnf-in up",
        "docker exec vnf ip addr add 192.168.17.1/24 dev vnf-in",
        "ip link add vnf-out type veth peer name vnf-out-root",
        "ip link set vnf-out-root up",
        "ip link set vnf-out netns {}".format(c_vnf_pid),
        "docker exec vnf ip link set vnf-out up",
        "docker exec vnf ip addr add 192.168.18.1/24 dev vnf-out",
        "ip link add pktgen-out type veth peer name pktgen-out-root",
        "ip link set pktgen-out-root up",
        "ip link set pktgen-out netns {}".format(c_pktgen_pid),
        "docker exec pktgen ip link set pktgen-out up",
        "docker exec pktgen ip addr add 192.168.17.2/24 dev pktgen-out",
        "ip link add pktgen-in type veth peer name pktgen-in-root",
        "ip link set pktgen-in-root up",
        "ip link set pktgen-in netns {}".format(c_pktgen_pid),
        "docker exec pktgen ip link set pktgen-in up",
        "docker exec pktgen ip addr add 192.168.18.2/24 dev pktgen-in",
    ]
    for c in cmds:
        run(split(c), check=True)

    client = docker.from_env()
    c_vnf = client.containers.list(
        all=True, filters={"label": "group=ffpp-dev-benchmark", "name": "vnf"}
    )[0]
    c_pktgen = client.containers.list(
        all=True, filters={"label": "group=ffpp-dev-benchmark", "name": "pktgen"}
    )[0]

    _, out = c_vnf.exec_run(cmd="cat /sys/class/net/vnf-in/address")
    vnf_in_mac = out.decode("ascii").strip()
    _, out = c_vnf.exec_run(cmd="cat /sys/class/net/vnf-out/address")
    vnf_out_mac = out.decode("ascii").strip()
    _, out = c_pktgen.exec_run(cmd="cat /sys/class/net/pktgen-in/address")
    pktgen_in_mac = out.decode("ascii").strip()
    _, out = c_pktgen.exec_run(cmd="cat /sys/class/net/pktgen-out/address")
    pktgen_out_mac = out.decode("ascii").strip()

    print("- Add permanent static ARP entries.")
    exit_code, _ = c_pktgen.exec_run(
        cmd="ip neigh add 192.168.17.1 lladdr {} dev pktgen-out nud permanent".format(
            vnf_in_mac
        )
    )
    if exit_code != 0:
        print("ERR: Faild to add static ARP.")
    exit_code, _ = c_vnf.exec_run(
        cmd="ip neigh add 192.168.18.2 lladdr {} dev vnf-out nud permanent".format(
            pktgen_in_mac
        )
    )
    if exit_code != 0:
        print("ERR: Faild to add static ARP.")

    client.close()

    with open("/sys/class/net/vnf-in-root/address", "r") as f:
        vnf_in_root_mac = f.read().strip()
    with open("/sys/class/net/pktgen-in-root/address", "r") as f:
        pktgen_in_root_mac = f.read().strip()

    print("- The MAC address of vnf-in in vnf: {}".format(vnf_in_mac))
    print("- The MAC address of vnf-out in vnf: {}".format(vnf_out_mac))
    print("- The MAC address of pktgen-in in pktgen: {}".format(pktgen_in_mac))
    print("- The MAC address of pktgen-out in pktgen: {}".format(pktgen_out_mac))

    print("- The MAC address of vnf-in-root: {}".format(vnf_in_root_mac))
    print("- The MAC address of pktgen-in-root: {}".format(pktgen_in_root_mac))

    print("* Run xdp_fwd programs for interfaces in the root namespace")
    xdp_fwd_prog_dir = os.path.abspath("../kern/xdp_fwd/")
    if not os.path.exists(xdp_fwd_prog_dir):
        print("ERR: Can not find the xdp_fwd program.")
        sys.exit(1)
    os.chdir(xdp_fwd_prog_dir)
    if not os.path.exists(os.path.join(xdp_fwd_prog_dir, "./xdp_fwd_kern.o")):
        print("\tINFO: Compile programs in xdp_fwd_kern.")
        run(split("make"), check=True)
    print("\t- Load xdp_fwd kernel program.")
    run(split("./xdp_fwd_loader pktgen-out-root"), check=True)
    # INFO/BUG: Currently. Ingress and egress traffic of VNF is all handled by the
    # vnf-in interface. Because the XDP-hock is added ONLY to the RX path in the
    # vnf, so DPDK application can not currently bind vnf-out inteface if AF_XDP
    # PMD is used.
    run(split("./xdp_fwd_loader vnf-in-root"), check=True)
    print("\t- Add forwarding between pktgen-out-root to vnf-in-root")
    run(
        split(
            "./xdp_fwd_user -i pktgen-out-root -r vnf-in-root -s {} -d {} -w {}".format(
                pktgen_out_mac, vnf_in_mac, vnf_in_root_mac
            )
        )
    )
    print("\t- Add forwarding between vnf-in-root to pktgen-in-root")
    run(
        split(
            "./xdp_fwd_user -i vnf-in-root -r pktgen-in-root -s {} -d {} -w {}".format(
                vnf_in_mac, pktgen_in_mac, pktgen_in_root_mac
            )
        )
    )

    print("* Current XDP status: ")
    run(split("xdp-loader status"), check=True)
    print(
        "* Setup finished. Run 'docker attach ffpp-dev-vnf' (or pktgen) to attach to the running containers."
    )


def teardown_common():
    client = docker.from_env()
    c_list = client.containers.list(
        all=True, filters={"label": "group=ffpp-dev-benchmark"}
    )
    for c in c_list:
        print("* Remove container: {}".format(c.name))
        c.remove(force=True)
    client.close()


def teardown_two_veth_xdp_fwd():
    teardown_common()
    bpf_map_dir = os.path.join(BPF_MAP_BASEDIR, "pktgen-out-root")
    if os.path.exists(bpf_map_dir):
        print("- Remove BPF maps directory: {}".format(bpf_map_dir))
        shutil.rmtree(bpf_map_dir)
    bpf_map_dir = os.path.join(BPF_MAP_BASEDIR, "vnf-in-root")
    if os.path.exists(bpf_map_dir):
        print("- Remove BPF maps directory: {}".format(bpf_map_dir))
        shutil.rmtree(bpf_map_dir)


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
        "--setup_name",
        type=str,
        default="two_direct_veth",
        choices=["two_direct_veth", "two_veth_xdp_fwd"],
        help="The name of the setup profile for benchmarking.",
    )
    parser.add_argument(
        "--pktgen_image",
        type=str,
        default=FFPP_DEV_CONTAINER_OPTS_DEFAULT["image"],
        help="The Docker image to create the pktgen.",
    )

    args = parser.parse_args()
    if args.action == "setup":
        print("The setup name: {}.".format(args.setup_name))
        if args.setup_name == "two_direct_veth":
            setup_two_direct_veth(args.pktgen_image)
        elif args.setup_name == "two_veth_xdp_fwd":
            setup_two_veth_xdp_fwd(args.pktgen_image)
    elif args.action == "teardown":
        if args.setup_name == "two_direct_veth":
            teardown_common()
        elif args.setup_name == "two_veth_xdp_fwd":
            teardown_two_veth_xdp_fwd()
