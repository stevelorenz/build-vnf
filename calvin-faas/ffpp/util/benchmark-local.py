#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Minimal local benchmark setups on a single machine using Docker containers.

       Only used to prototype and test functionalities of programs, not designed
       for latency performance.
"""

import argparse
import multiprocessing
import os
import shlex
import shutil
import subprocess
import sys
import time

import docker

with open("../VERSION", "r") as vfile:
    FFPP_VER = vfile.read().strip()

PARENT_DIR = os.path.abspath(os.path.join(os.path.curdir, os.pardir))
TREX_CONF_DIR = PARENT_DIR + "/benchmark/trex"
BPF_MAP_BASEDIR = "/sys/fs/bpf"
LOCKED_IN_MEMORY_SIZE = 65536  # kbytes

TREX_CONF = {
    TREX_CONF_DIR + "/trex_cfg.yaml": {"bind": "/etc/trex_cfg.yaml", "mode": "rw"},
    TREX_CONF_DIR: {"bind": "/trex/v2.81/local", "mode": "rw"},
}

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
    "working_dir": "/ffpp",
    "image": "ffpp-dev:%s" % (FFPP_VER),
    "command": "bash",
    "labels": {"group": "ffpp-dev-benchmark"},
    # Ulimits, memlock should be enough to load eBPF maps.
    "ulimits": [
        docker.types.Ulimit(
            name="memlock", hard=LOCKED_IN_MEMORY_SIZE, soft=LOCKED_IN_MEMORY_SIZE
        )
    ],
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
    pktgen_args["volumes"].update(TREX_CONF)
    c_pktgen = client.containers.run(**pktgen_args)
    while not c_pktgen.attrs["State"]["Running"]:
        time.sleep(0.05)
        c_pktgen.reload()
    c_pktgen_pid = c_pktgen.attrs["State"]["Pid"]
    c_pktgen.exec_run("mount -t bpf bpf /sys/fs/bpf/")
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
        "docker exec vnf ip addr add 192.168.17.2/24 dev vnf-in",
        "docker exec pktgen ip addr add 192.168.17.1/24 dev pktgen-out",
        "ip link add vnf-out type veth peer name pktgen-in",
        "ip link set vnf-out up",
        "ip link set pktgen-in up",
        "ip link set vnf-out netns {}".format(c_vnf_pid),
        "ip link set pktgen-in netns {}".format(c_pktgen_pid),
        "docker exec vnf ip link set vnf-out up",
        "docker exec pktgen ip link set pktgen-in up",
        "docker exec vnf ip addr add 192.168.18.2/24 dev vnf-out",
        "docker exec pktgen ip addr add 192.168.18.1/24 dev pktgen-in",
        "docker exec pktgen ping -q -c 2 192.168.17.2",
        "docker exec pktgen ping -q -c 2 192.168.18.2",
    ]
    for c in cmds:
        subprocess.run(shlex.split(c), check=True)

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
        "docker exec vnf ip addr add 192.168.17.2/24 dev vnf-in",
        "ip link add vnf-out type veth peer name vnf-out-root",
        "ip link set vnf-out-root up",
        "ip link set vnf-out netns {}".format(c_vnf_pid),
        "docker exec vnf ip link set vnf-out up",
        "docker exec vnf ip addr add 192.168.18.2/24 dev vnf-out",
        "ip link add pktgen-out type veth peer name pktgen-out-root",
        "ip link set pktgen-out-root up",
        "ip link set pktgen-out netns {}".format(c_pktgen_pid),
        "docker exec pktgen ip link set pktgen-out up",
        "docker exec pktgen ip addr add 192.168.17.1/24 dev pktgen-out",
        "ip link add pktgen-in type veth peer name pktgen-in-root",
        "ip link set pktgen-in-root up",
        "ip link set pktgen-in netns {}".format(c_pktgen_pid),
        "docker exec pktgen ip link set pktgen-in up",
        "docker exec pktgen ip addr add 192.168.18.1/24 dev pktgen-in",
    ]
    for c in cmds:
        subprocess.run(shlex.split(c), check=True)

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

    print("* Load xdp-pass on ingress and egress interfaces inside vnf and pktgen.")

    for c, c_name in zip([c_vnf, c_pktgen], ["vnf", "pktgen"]):
        exit_code, out = c.exec_run(cmd="make", workdir="/ffpp/kern/xdp_pass",)
        if exit_code != 0:
            print("ERR: Faild to compile xdp-pass program in {}.".format(c_name))
            sys.exit(1)
        for iface_suffix in ["in", "out"]:
            iface_name = "{}-{}".format(c_name, iface_suffix)
            exit_code, out = c.exec_run(
                cmd="xdp-loader load -m native {} ./xdp_pass_kern.o".format(iface_name),
                workdir="/ffpp/kern/xdp_pass",
            )
            if exit_code != 0:
                print(
                    "ERR: Faild to load xdp-pass programs in {} on interface:{}.".format(
                        c_name, iface_name
                    )
                )
                sys.exit(1)

    with open("/sys/class/net/vnf-in-root/address", "r") as f:
        vnf_in_root_mac = f.read().strip()
    with open("/sys/class/net/vnf-out-root/address", "r") as f:
        vnf_out_root_mac = f.read().strip()
    with open("/sys/class/net/pktgen-in-root/address", "r") as f:
        pktgen_in_root_mac = f.read().strip()
    with open("/sys/class/net/pktgen-out-root/address", "r") as f:
        pktgen_out_root_mac = f.read().strip()

    print("- The MAC address of vnf-in in vnf: {}".format(vnf_in_mac))
    print("- The MAC address of vnf-out in vnf: {}".format(vnf_out_mac))
    print("- The MAC address of pktgen-in in pktgen: {}".format(pktgen_in_mac))
    print("- The MAC address of pktgen-out in pktgen: {}".format(pktgen_out_mac))

    print("- The MAC address of vnf-in-root: {}".format(vnf_in_root_mac))
    print("- The MAC address of vnf-out-root: {}".format(vnf_out_root_mac))
    print("- The MAC address of pktgen-in-root: {}".format(pktgen_in_root_mac))
    print("- The MAC address of pktgen-out-root: {}".format(pktgen_out_root_mac))

    print("* Run xdp_fwd programs for interfaces in the root namespace")
    xdp_fwd_prog_dir = os.path.abspath("../kern/xdp_fwd/")
    if not os.path.exists(xdp_fwd_prog_dir):
        print("ERR: Can not find the xdp_fwd program.")
        sys.exit(1)
    os.chdir(xdp_fwd_prog_dir)
    if not os.path.exists(os.path.join(xdp_fwd_prog_dir, "./xdp_fwd_kern.o")):
        print("\tINFO: Compile programs in xdp_fwd_kern.")
        subprocess.run(shlex.split("make"), check=True)
    print("\t- Load xdp_fwd kernel programs.")
    subprocess.run(shlex.split("./xdp_fwd_loader vnf-in-root"), check=True)
    subprocess.run(shlex.split("./xdp_fwd_loader vnf-out-root"), check=True)
    subprocess.run(shlex.split("./xdp_fwd_loader pktgen-in-root"), check=True)
    subprocess.run(shlex.split("./xdp_fwd_loader pktgen-out-root"), check=True)

    print("\t- Add forwarding between pktgen-out-root to vnf-in-root")
    subprocess.run(
        shlex.split(
            "./xdp_fwd_user -i pktgen-out-root -r vnf-in-root -s {} -d {} -w {}".format(
                pktgen_out_mac, vnf_in_mac, vnf_in_root_mac
            )
        )
    )
    subprocess.run(
        shlex.split(
            "./xdp_fwd_user -i vnf-in-root -r pktgen-out-root -s {} -d {} -w {}".format(
                vnf_in_mac, pktgen_out_mac, pktgen_out_root_mac
            )
        )
    )
    print("\t- Add forwarding between vnf-out-root to pktgen-in-root")
    subprocess.run(
        shlex.split(
            "./xdp_fwd_user -i vnf-out-root -r pktgen-in-root -s {} -d {} -w {}".format(
                vnf_out_mac, pktgen_in_mac, pktgen_in_root_mac
            )
        )
    )
    subprocess.run(
        shlex.split(
            "./xdp_fwd_user -i pktgen-in-root -r vnf-out-root -s {} -d {} -w {}".format(
                pktgen_in_mac, vnf_out_mac, vnf_out_root_mac
            )
        )
    )

    print("* Current XDP status in root namespace: ")
    subprocess.run(shlex.split("xdp-loader status"), check=True)

    client.close()

    print(
        "* Setup finished. Run 'docker attach ffpp-dev-vnf' (or pktgen) to attach to the running containers."
    )


def setup_two_cnfs_veth_xdp_fwd(pktgen_image):
    pass


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
    for d in ["vnf-in-root", "vnf-out-root", "pktgen-in-root", "pktgen-out-root"]:
        bpf_map_dir = os.path.join(BPF_MAP_BASEDIR, d)
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
    nano_cpus_help = " ".join(
        (
            "CPU quota in units of 1e-9 CPUs.",
            "For example, 5e8 means 50%% of the CPU.",
            "WARN: the type of this argument is float to allow XeX format,",
            "it is converted to integer.",
        )
    )
    parser.add_argument("--nano_cpus", type=float, default=5e8, help=nano_cpus_help)

    args = parser.parse_args()

    FFPP_DEV_CONTAINER_OPTS_DEFAULT["nano_cpus"] = int(args.nano_cpus)

    cpu_count = multiprocessing.cpu_count()
    if cpu_count < 2:
        print("ERR: The setup needs at least 2 CPU cores.")
        sys.exit(1)

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
