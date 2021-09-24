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


CURRENT_DIR = os.path.abspath(os.path.curdir)
DIMAGE = "coin_yolo:0.0.1"


class Test(object):
    def __init__(self):
        self.net = Containernet(
            controller=Controller,
            link=TCLink,
            xterms=False,
        )

        self._vnfs = []
        self._switches = []

    def setup(self):
        info("*** Adding controller\n")
        self.net.addController(
            "c0", controller=RemoteController, port=6653, protocols="OpenFlow13"
        )
        # MARK: Host addresses below 11 could be used for network services.
        info("*** Adding end hosts\n")
        self.client = self.net.addDockerHost(
            "client",
            dimage=f"{DIMAGE}",
            ip="10.0.1.11/16",
            docker_args={
                "cpuset_cpus": "0",
                "hostname": "client",
                "working_dir": "/coin_yolo",
                "volumes": {
                    CURRENT_DIR: {"bind": "/coin_yolo/share", "mode": "rw"},
                },
            },
        )

        self.server = self.net.addDockerHost(
            "server",
            dimage=f"{DIMAGE}",
            ip="10.0.3.11/16",
            docker_args={
                "cpuset_cpus": "0",
                "hostname": "server",
                "working_dir": "/coin_yolo",
                "volumes": {
                    CURRENT_DIR: {"bind": "/coin_yolo/share", "mode": "rw"},
                },
            },
        )

    def run_multi_htop(self, node_num, vnf_type, vnf_mode):
        info("* Running multi_hop test.\n")

        info("*** Adding network nodes.\n")
        host_addr_base = 10
        for n in range(1, node_num + 1):
            vnf = self.net.addDockerHost(
                f"vnf{n}",
                dimage=f"{DIMAGE}",
                ip=f"10.0.2.{host_addr_base+n}/16",
                docker_args={
                    "cpuset_cpus": "1",
                    # "nano_cpus": int(math.floor(1e9 / node_num)),
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
                        CURRENT_DIR: {"bind": "/coin_yolo/share", "mode": "rw"},
                    },
                    "working_dir": "/coin_yolo",
                },
            )
            self._vnfs.append(vnf)
            self._switches.append(self.net.addSwitch(f"s{n}", protocols="OpenFlow13"))

        info("*** Creating links.\n")
        # TODO: Discuss about the link parameters
        self.net.addLinkNamedIfce(self._switches[0], self.client, bw=1000, delay="10ms")
        self.net.addLinkNamedIfce(
            self._switches[-1], self.server, bw=1000, delay="10ms"
        )
        # For network nodes
        for n in range(0, node_num - 1):
            self.net.addLink(
                self._switches[n], self._switches[n + 1], bw=1000, delay="10ms"
            )
        for i, s in enumerate(self._switches):
            self.net.addLinkNamedIfce(s, self._vnfs[i], bw=1000, delay="1ms")

        self.net.start()

        c0 = self.net.get("c0")
        makeTerm(c0, cmd="ryu-manager ./multi_hop_controller.py ; read")

        # Avoid ARP storm (ARP is not implemented in FFPP...)
        # Let all VNF nodes work on layer 2.
        info("*** Flush the IP address of VNF's data plane interface.\n")
        for idx, v in enumerate(self._vnfs):
            v.cmd(f"ip addr flush vnf{idx+1}-s{idx+1}")

        self.warm_up()

        info("*** Ping server from client.\n")
        ret = self.client.cmd(f"ping -c 3 {self.server.IP()}")
        print(ret)
        info("*** Ping client from server.\n")
        ret = self.server.cmd(f"ping -c 3 {self.client.IP()}")
        print(ret)

        # TODO: Run VNFs with proper parameters
        # print(f"*** Deploy VNFs, VNF type:{vnf_type}, VNF mode: {vnf_mode}")

        # vnf_type_map = {"meica": "./build/meica_vnf", "cnn": "./build/cnn_vnf"}
        # vnf_bin = vnf_type_map[vnf_type]

        # if vnf_mode == "null":
        #     return
        # elif vnf_mode == "store_forward":
        #     for v in self._vnfs:
        #         v.cmd(
        #             f"cd /coin_yolo/emulation && {vnf_bin} --mode store_forward & 2>&1"
        #         )
        #         time.sleep(1)  # Avoid memory corruption among VNFs.
        # elif vnf_mode == "compute_forward":
        #     v = self._vnfs[0]
        #     # Max rounds are not used in cnn.
        #     v.cmd(
        #         f"cd /coin_yolo/emulation && {vnf_bin} --mode compute_forward --leader --max_rounds {max_rounds} & 2>&1"
        #     )
        #     for v in self._vnfs[1:]:
        #         v.cmd(
        #             f"cd /coin_yolo/emulation && {vnf_bin} --mode compute_forward --max_rounds {max_rounds} & 2>&1"
        #         )

    def warm_up(self):
        self.client.cmd(f"ping -c 1 {self.server.IP()}")
        self.server.cmd(f"ping -c 1 {self.client.IP()}")

    def run(self, topo, node_num, vnf_type, vnf_mode):
        if topo == "multi_hop":
            self.run_multi_htop(node_num, vnf_type, vnf_mode)


if __name__ == "__main__":
    if os.geteuid() != 0:
        print("Run this script with sudo.", file=sys.stderr)
        sys.exit(1)
    parser = argparse.ArgumentParser(description="Process some integers.")
    parser.add_argument(
        "--topo",
        type=str,
        default="multi_hop",
        choices=["multi_hop"],
        help="Name of the test topology",
    )
    parser.add_argument(
        "--node_num",
        type=int,
        default=3,
        help="Number of network nodes in the topology",
    )
    parser.add_argument(
        "--vnf_type",
        type=str,
        default="yolo",
        choices=["yolo"],
        help="Type of the VNF for deployment",
    )
    parser.add_argument(
        "--vnf_mode",
        type=str,
        default="store_forward",
        choices=["null", "store_forward", "compute_forward"],
        help="Mode to run all VNFs.",
    )
    args = parser.parse_args()

    home_dir = os.path.expanduser("~")
    xresources_path = os.path.join(home_dir, ".Xresources")
    if os.path.exists(xresources_path):
        subprocess.run(shlex.split(f"xrdb -merge {xresources_path}"), check=True)

    # IPv6 is currently not used.
    subprocess.run(
        shlex.split("sysctl -w net.ipv6.conf.all.disable_ipv6=1"),
        check=True,
    )

    setLogLevel("info")
    test = Test()
    test.setup()

    try:
        test.run(
            topo=args.topo,
            node_num=args.node_num,
            vnf_type=args.vnf_type,
            vnf_mode=args.vnf_mode,
        )
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
