#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Network topology to test COIN YOLO
"""

import argparse
import math
import os
import shlex
import subprocess
import sys
import time

from comnetsemu.cli import CLI
from comnetsemu.net import Containernet
from mininet.link import TCLink
from mininet.log import info, setLogLevel
from mininet.node import Controller, RemoteController
from mininet.term import makeTerm

import config


CURRENT_DIR = os.path.abspath(os.path.curdir)
DIMAGE = "coin_dl:latest"


def check_env():
    if os.cpu_count() < 3:
        raise RuntimeError("Requires at least 3 CPUs")


class Test:
    """Test"""

    def __init__(self, topo: str, node_num: int, vnf_mode: str, dev: bool):
        self.topo = topo
        if self.topo == "multi_hop" and node_num < 1:
            raise RuntimeError("Require at least ONE network node!")
        self.node_num = node_num
        self.vnf_mode = vnf_mode
        self.dev = dev

        self.net = Containernet(
            controller=Controller,
            link=TCLink,
            xterms=False,
        )

        self._vnfs = []
        self._switches = []

        self.setup()

    def setup(self):
        info("*** Adding controller\n")
        self.net.addController(
            "c0", controller=RemoteController, port=6653, protocols="OpenFlow13"
        )

        if self.topo == "multi_hop":
            self.setup_multi_hop()

    def setup_multi_hop(self):
        info("*** Adding end hosts\n")
        # TODO: Extend this to multiple end hosts ?
        # TODO: Add a noisy neighbor host if there's no gain with compute-and-forwar
        # TODO: Check the cpuset_cpus and allocate them on different sockets
        client_cpu_args = config.end_host_cpu_args["client"]
        self.client = self.net.addDockerHost(
            "client",
            dimage=f"{DIMAGE}",
            ip="10.0.1.11/16",
            docker_args={
                "cpuset_cpus": client_cpu_args["cpuset_cpus"],
                "nano_cpus": client_cpu_args["nano_cpus"],
                "hostname": "client",
                "working_dir": "/coin_dl",
                "volumes": {
                    CURRENT_DIR: {"bind": "/coin_dl/share", "mode": "rw"},
                },
            },
        )

        server_cpu_args = config.end_host_cpu_args["server"]
        self.server = self.net.addDockerHost(
            "server",
            dimage=f"{DIMAGE}",
            ip="10.0.3.11/16",
            docker_args={
                "cpuset_cpus": server_cpu_args["cpuset_cpus"],
                "nano_cpus": server_cpu_args["nano_cpus"],
                "hostname": "server",
                "working_dir": "/coin_dl",
                "volumes": {
                    CURRENT_DIR: {"bind": "/coin_dl/share", "mode": "rw"},
                },
            },
        )

        nano_cpus_vnf = int(math.floor(1e9 / self.node_num))
        if self.dev:
            nano_cpus_vnf = int(5e8)  # 50%
        info("*** Adding network node(s).\n")
        info(f"VNF nano_cpus: {nano_cpus_vnf}\n")
        host_addr_base = 10
        for n in range(1, self.node_num + 1):
            vnf = self.net.addDockerHost(
                f"vnf{n}",
                dimage=f"{DIMAGE}",
                ip=f"10.0.2.{host_addr_base+n}/16",
                docker_args={
                    "cpuset_cpus": "2",
                    "nano_cpus": nano_cpus_vnf,
                    "hostname": f"vnf{n}",
                    # For DPDK-related resources.
                    "volumes": {
                        "/sys/bus/pci/drivers": {
                            "bind": "/sys/bus/pci/drivers",
                            "mode": "rw",
                        },
                        "/sys/kernel/mm/hugepages": {
                            "bind": "/sys/kernel/mm/hugepages",
                            "mode": "rw",
                        },
                        "/sys/devices/system/node": {
                            "bind": "/sys/devices/system/node",
                            "mode": "rw",
                        },
                        "/dev": {"bind": "/dev", "mode": "rw"},
                        CURRENT_DIR: {"bind": "/coin_dl/share", "mode": "rw"},
                    },
                    "working_dir": "/coin_dl",
                },
            )
            self._vnfs.append(vnf)
            self._switches.append(self.net.addSwitch(f"s{n}", protocols="OpenFlow13"))

        link_params = config.link_params
        info("*** Creating links.\n")
        self.net.addLinkNamedIfce(
            self._switches[0],
            self.client,
            bw=link_params["host-sw"]["bw"],
            delay=link_params["host-sw"]["delay"],
        )
        self.net.addLinkNamedIfce(
            self._switches[-1],
            self.server,
            bw=link_params["host-sw"]["bw"],
            delay=link_params["host-sw"]["delay"],
        )
        # For network nodes
        for n in range(0, self.node_num - 1):
            self.net.addLink(
                self._switches[n],
                self._switches[n + 1],
                bw=link_params["sw-sw"]["bw"],
                delay=link_params["sw-sw"]["delay"],
            )

        for i, s in enumerate(self._switches):
            self.net.addLinkNamedIfce(
                s,
                self._vnfs[i],
                bw=link_params["sw-vnf"]["bw"],
                delay=link_params["sw-vnf"]["delay"],
            )

        self.net.start()

        c0 = self.net.get("c0")
        # MARK: this does not work with tty
        makeTerm(c0, cmd="ryu-manager ./multi_hop_controller.py ; read")

        # Avoid ARP storm (ARP is not implemented in VNF/FFPP...)
        # Let all VNF nodes work on layer 2.
        info("*** Flush the IP address of VNF's data plane interface.\n")
        for idx, v in enumerate(self._vnfs):
            v.cmd(f"ip addr flush vnf{idx+1}-s{idx+1}")

    def warm_up(self):
        self.client.cmd(f"ping -c 1 {self.server.IP()}")
        self.server.cmd(f"ping -c 1 {self.client.IP()}")

    def run_multi_hop(self, test):
        info(
            f"* Run measurements for multi-hop topology with VNF mode: {self.vnf_mode}, test: {test}\n"
        )
        # Deploy programs on VNFs
        if self.vnf_mode == "null":
            info("null mode! Deploy nothing on VNF(s)\n")
        elif self.vnf_mode == "store_forward":
            for vnf in self._vnfs:
                vnf.cmd("./build/coin_dl &")
        elif self.vnf_mode == "compute_forward":
            print("Compute and Forward!!!")

        # Deploy programs on end hosts
        # Always run sockperf on the DEFAULT_PORT, can be used for e.g. noisy neighbor
        self.server.cmd(f"sockperf server -p {config.DEFAULT_PORT} --daemonize")
        if test == "sockperf":
            self.server.cmd(f"sockperf server -p {config.SFC_PORT} --daemonize")

        # Run measurements
        if test == "null":
            return
        elif test == "sockperf":
            print("*** Running sockperf measurements...")
            self.client.cmd("cd share")
            self.client.cmd("python3 ./run_sockperf.py -d 60 -r 2 -m 10000 -o result")
            print("*** Finished sockperf test")
        elif test == "coin_dl":
            print("AbaAba")
        elif test == "server_local":
            print("*** Running server local measurements...")
            ret = self.server.cmd("python3 ./server.py -m server_local")
            print(ret)

    def run(self, test: str):
        self.warm_up()
        info("*** Ping server from client.\n")
        ret = self.client.cmd(f"ping -c 3 {self.server.IP()}")
        print(ret)
        info("*** Ping client from server.\n")
        ret = self.server.cmd(f"ping -c 3 {self.client.IP()}")
        print(ret)

        if self.topo == "multi_hop":
            self.run_multi_hop(test)


def main():
    if os.geteuid() != 0:
        print("Run this script with sudo.", file=sys.stderr)
        sys.exit(1)

    parser = argparse.ArgumentParser(description="Network topology to test COIN YOLO")
    parser.add_argument(
        "--topo",
        type=str,
        default="multi_hop",
        choices=["multi_hop"],
        help="Name of the test topology",
    )
    parser.add_argument(
        "-n",
        "--node_num",
        type=int,
        default=1,
        help="Number of NETWORK node(s) in the topology",
    )
    parser.add_argument(
        "-m",
        "--vnf_mode",
        type=str,
        default="store_forward",
        # null is just for debug, nothing is deployed on VNF(s)
        choices=["null", "store_forward", "compute_forward"],
        help="Working mode of all VNF(s)",
    )
    parser.add_argument(
        "--test",
        type=str,
        default="sockperf",
        choices=["null", "sockperf", "coin_dl", "server_local"],
        help="The test to be performed",
    )
    parser.add_argument(
        "--dev", action="store_true", help="Run the setup for dev purpose on laptop"
    )
    args = parser.parse_args()

    check_env()
    setLogLevel("info")

    # IPv6 is currently not used, disable it.
    subprocess.run(
        shlex.split("sysctl -w net.ipv6.conf.all.disable_ipv6=1"),
        check=True,
    )

    # ISSUE: The code here is not exception safe! Fix it later!
    try:
        test = Test(args.topo, args.node_num, args.vnf_mode, args.dev)
        test.run(args.test)
        if args.dev:
            info("*** Enter CLI\n")
            CLI(test.net)
    finally:
        info("*** Stopping network")
        test.net.stop()
        subprocess.run(shlex.split("sudo killall ryu-manager"), check=True)
        subprocess.run(
            shlex.split("sysctl -w net.ipv6.conf.all.disable_ipv6=0"),
            check=True,
        )


if __name__ == "__main__":
    main()
