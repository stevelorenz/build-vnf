#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Minimal local benchmark setups on a single machine using Docker containers.

       Only used to prototype (code is verbose (duplicated) and not optimized)
       and test functionalities of programs, not designed for latency
       performance , configurability and scalability.
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
    # Ulimits, memlock MUST be enough to load eBPF maps.
    "ulimits": [
        docker.types.Ulimit(
            name="memlock", hard=LOCKED_IN_MEMORY_SIZE, soft=LOCKED_IN_MEMORY_SIZE
        )
    ],
}


def add_veth_pairs(vnf_name, vnf_pid, suffix):
    for direct in ["in", "out"]:
        cmds = [
            f"ip link add vnf-{direct} type veth peer name vnf{suffix}-{direct}-root",
            f"ip link set vnf{suffix}-{direct}-root up",
            f"ip link set vnf-{direct} netns {vnf_pid}",
            f"docker exec {vnf_name} ip link set vnf-{direct} up",
        ]
        for c in cmds:
            subprocess.run(shlex.split(c), check=True)


def setup_containers(pktgen_image, cpu_count, vnf_per_core):
    client = docker.from_env()
    c_vnfs = list()
    c_vnfs_name = list()
    c_vnfs_pid = list()

    # The CPU 0 is used by the pktgen
    pktgen_args = FFPP_DEV_CONTAINER_OPTS_DEFAULT.copy()
    pktgen_args["name"] = "pktgen"
    pktgen_args["image"] = pktgen_image
    pktgen_args["volumes"].update(TREX_CONF)
    pktgen_args["cpuset_cpus"] = "0"
    c_pktgen = client.containers.run(**pktgen_args)
    while not c_pktgen.attrs["State"]["Running"]:
        time.sleep(0.05)
        c_pktgen.reload()
    c_pktgen_pid = c_pktgen.attrs["State"]["Pid"]
    c_pktgen.exec_run("mount -t bpf bpf /sys/fs/bpf/")
    print("- Pktgen container is running with PID:{}.".format(c_pktgen_pid))

    # The CPU 1 is used by the default gateway vnf0.
    vnf_num = 0
    vnf_args = FFPP_DEV_CONTAINER_OPTS_DEFAULT.copy()
    vnf_args["name"] = f"vnf{vnf_num}"
    c_vnfs_name.append(f"vnf{vnf_num}")
    c_vnfs.append(client.containers.run(**vnf_args))
    vnf_num += 1

    for c in range(2, cpu_count):
        for v in range(vnf_per_core):
            vnf_args = FFPP_DEV_CONTAINER_OPTS_DEFAULT.copy()
            vnf_args["name"] = f"vnf{c}_{v}"
            c_vnfs_name.append(f"vnf{c}_{v}")
            vnf_args["cpuset_cpus"] = f"{c}"
            print(f"- Run VNF container {c}_{v} on core: {c}")
            c_vnfs.append(client.containers.run(**vnf_args))
            vnf_num += 1

    for c in c_vnfs:
        while not c.attrs["State"]["Running"]:
            time.sleep(0.05)
            c.reload()
        pid = c.attrs["State"]["Pid"]
        c_vnfs_pid.append(pid)
        c.exec_run("mount -t bpf bpf /sys/fs/bpf")
        print(f"- VNF container with name: {c.name} and PID: {pid} is running.")

    client.close()

    return (c_vnfs_name, c_vnfs_pid, c_pktgen_pid)


def setup_veth_xdp_fwd(pktgen_image, cpu_count, vnf_per_core, vnf_veth_pairs):
    c_vnfs_name, c_vnf_pid, c_pktgen_pid = setup_containers(
        pktgen_image, cpu_count, vnf_per_core
    )
    # ONLY vnf0 container has network data plane interfaces.
    cmds = [
        "mkdir -p /var/run/netns",
        f"ln -s /proc/{c_vnf_pid[0]}/ns/net /var/run/netns/{c_vnf_pid[0]}",
        f"ln -s /proc/{c_pktgen_pid}/ns/net /var/run/netns/{c_pktgen_pid}",
        "ip link add vnf-in type veth peer name vnf-in-root",
        "ip link set vnf-in-root up",
        f"ip link set vnf-in netns {c_vnf_pid[0]}",
        "docker exec vnf0 ip link set vnf-in up",
        "docker exec vnf0 ip addr add 192.168.17.2/24 dev vnf-in",
        "ip link add vnf-out type veth peer name vnf-out-root",
        "ip link set vnf-out-root up",
        f"ip link set vnf-out netns {c_vnf_pid[0]}",
        "docker exec vnf0 ip link set vnf-out up",
        "docker exec vnf0 ip addr add 192.168.18.2/24 dev vnf-out",
        "ip link add pktgen-out type veth peer name pktgen-out-root",
        "ip link set pktgen-out-root up",
        f"ip link set pktgen-out netns {c_pktgen_pid}",
        "docker exec pktgen ip link set pktgen-out up",
        "docker exec pktgen ip addr add 192.168.17.1/24 dev pktgen-out",
        "ip link add pktgen-in type veth peer name pktgen-in-root",
        "ip link set pktgen-in-root up",
        f"ip link set pktgen-in netns {c_pktgen_pid}",
        "docker exec pktgen ip link set pktgen-in up",
        "docker exec pktgen ip addr add 192.168.18.1/24 dev pktgen-in",
    ]
    for c in cmds:
        subprocess.run(shlex.split(c), check=True)

    if vnf_veth_pairs:
        print(
            "* Add additional data plane interfaces to containers: {}".format(
                ", ".join(c_vnfs_name[1:])
            )
        )
        s = 1
        for n, p in zip(c_vnfs_name[1:], c_vnf_pid[1:]):
            add_veth_pairs(n, p, s)
            s += 1

    client = docker.from_env()
    c_vnf = client.containers.list(
        all=True, filters={"label": "group=ffpp-dev-benchmark", "name": "vnf0"}
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

    # Required to forward packets using AF_XDP.
    print("* Load xdp-pass on ingress and egress interfaces inside vnf0 and pktgen.")
    for c, c_name in zip([c_vnf, c_pktgen], ["vnf", "pktgen"]):
        exit_code, out = c.exec_run(
            cmd="make",
            workdir="/ffpp/kern/xdp_pass",
        )
        if exit_code != 0:
            print("ERROR: Faild to compile xdp-pass program in {}.".format(c_name))
            sys.exit(1)
        for iface_suffix in ["in", "out"]:
            iface_name = "{}-{}".format(c_name, iface_suffix)
            exit_code, out = c.exec_run(
                cmd="xdp-loader load -m native {} ./xdp_pass_kern.o".format(iface_name),
                workdir="/ffpp/kern/xdp_pass",
            )
            if exit_code != 0:
                print(
                    "ERROR: Faild to load xdp-pass programs in {} on interface:{}.".format(
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

    # Temporary solution to mimic a SmartNIC or SmartSwitch with XDP support.
    # The XDP/AF_XDP support of mainline OvS is still experimental now, replace
    # following pasta code with OvS when the support is stable.
    print("* Run xdp_fwd programs for interfaces in the root namespace.")
    xdp_fwd_prog_dir = os.path.abspath("../kern/xdp_fwd/")
    if not os.path.exists(xdp_fwd_prog_dir):
        print("ERROR: Can not find the xdp_fwd program.")
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

    print("\t- Add forwarding between pktgen-out-root to vnf-in-root.")
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
        "* Setup finished. Run 'docker attach vnf0' (or pktgen) to attach to the running containers."
    )


def teardown_containers():
    client = docker.from_env()
    c_list = client.containers.list(
        all=True, filters={"label": "group=ffpp-dev-benchmark"}
    )
    for c in c_list:
        print("* Remove container: {}".format(c.name))
        c.remove(force=True)
    client.close()


def teardown_veth_xdp_fwd():
    teardown_containers()
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
        choices=["setup", "teardown", "test"],
        help="The action to perform.",
    )
    parser.add_argument(
        "--setup_name",
        type=str,
        default="veth_xdp_fwd",
        choices=["veth_xdp_fwd"],
        help="The name of the setup profile for benchmarking.",
    )
    parser.add_argument(
        "--pktgen_image",
        type=str,
        default=FFPP_DEV_CONTAINER_OPTS_DEFAULT["image"],
        help="The Docker image to create the pktgen.",
    )
    NANO_CPUS_HELP = " ".join(
        (
            "CPU quota in units of 1e-9 CPUs.",
            "For example, 5e8 means 50%% of the CPU.",
            "WARN: the type of this argument is float to allow XeX format,",
            "it is converted to integer.",
        )
    )
    parser.add_argument("--nano_cpus", type=float, default=5e8, help=NANO_CPUS_HELP)
    parser.add_argument(
        "--vnf_per_core",
        type=int,
        default=1,
        help="The number of co-located VNFs on a single worker CPU core.",
    )
    parser.add_argument(
        "--vnf_veth_pairs",
        action="store_true",
        default=False,
        help="Add additional veth pairs for all VNFs.",
    )

    args = parser.parse_args()

    FFPP_DEV_CONTAINER_OPTS_DEFAULT["nano_cpus"] = int(args.nano_cpus)

    cpu_count = multiprocessing.cpu_count()
    if cpu_count < 2:
        print("ERROR: The setup needs at least 2 CPU cores.")
        sys.exit(1)

    if args.action == "setup":
        print("* The setup name: {}.".format(args.setup_name))
        if args.setup_name == "veth_xdp_fwd":
            setup_veth_xdp_fwd(
                args.pktgen_image, cpu_count, args.vnf_per_core, args.vnf_veth_pairs
            )
    elif args.action == "teardown":
        if args.setup_name == "veth_xdp_fwd":
            teardown_veth_xdp_fwd()
    elif args.action == "test":
        setup_veth_xdp_fwd(args.pktgen_image, cpu_count, 3, True)
        teardown_veth_xdp_fwd()
