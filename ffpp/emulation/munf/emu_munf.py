#! /usr/bin/env python3
# -*- coding: utf-8 -*-

"""
About: Emulation topology for MuNF system.
"""

import argparse
import os
import subprocess
import shlex
import sys
import time

from comnetsemu.cli import CLI
from comnetsemu.net import Containernet, VNFManager

from mininet.link import TCLink
from mininet.log import info, setLogLevel
from mininet.node import Controller

TREX_VER = "v2.81"
with open("../../VERSION", "r") as vfile:
    FFPP_VER = vfile.read().strip()

FFPP_DIR = os.path.abspath(os.path.join(os.path.curdir, "../../"))
TREX_CONF_DIR = os.path.join(FFPP_DIR + "/benchmark/trex")


def getOFPort(sw, ifce_name):
    """Get the openflow port based on iterface name"""
    return sw.vsctl(f"get Interface {ifce_name} ofport").strip()


def setupOvs(sw):
    dut_egress_port = getOFPort(sw, "s1-vnf-out")
    pktgen_ingress_port = getOFPort(sw, "s1-pktgen-in")
    info("*** S1 OpenFlow port information:\n")
    print(
        f"DUT out port number: {dut_egress_port}, Pktgen in port number: {pktgen_ingress_port}"
    )
    subprocess.run(
        shlex.split(
            'ovs-ofctl add-flow s1 "udp,in_port={},actions=output={}"'.format(
                dut_egress_port, pktgen_ingress_port
            )
        ),
        check=True,
    )


def testMuNF(nano_cpus):
    net = Containernet(controller=Controller, link=TCLink)
    mgr = VNFManager(net)

    start_ts = time.time()
    info("*** Adding controller\n")
    net.addController("c0")

    info("*** Adding Docker hosts\n")
    pktgen = net.addDockerHost(
        "pktgen",
        dimage=f"trex:{TREX_VER}",
        ip="10.0.0.1/24",
        docker_args={
            "cpuset_cpus": "0",
            "hostname": "pktgen",
            "volumes": {
                os.path.join(TREX_CONF_DIR, "trex_cfg.yaml"): {
                    "bind": "/etc/trex_cfg.yaml",
                    "mode": "rw",
                },
                TREX_CONF_DIR: {"bind": f"/trex/{TREX_VER}/local", "mode": "rw"},
            },
        },
    )

    dut = net.addDockerHost(
        "dut",
        dimage=f"ffpp:{FFPP_VER}",
        ip="10.0.0.2/24",
        docker_args={
            "cpuset_cpus": "1,2",
            "nano_cpus": int(nano_cpus),
            "hostname": "dut",
            "volumes": {
                "/sys/bus/pci/drivers": {"bind": "/sys/bus/pci/drivers", "mode": "rw"},
                "/sys/kernel/mm/hugepages": {
                    "bind": "/sys/kernel/mm/hugepages",
                    "mode": "rw",
                },
                "/sys/devices/system/node": {
                    "bind": "/sys/devices/system/node",
                    "mode": "rw",
                },
                "/dev": {"bind": "/dev", "mode": "rw"},
                FFPP_DIR: {"bind": "/ffpp", "mode": "rw"},
            },
        },
    )
    s1 = net.addSwitch("s1")

    # Control plane links.
    net.addLinkNamedIfce(s1, pktgen)
    net.addLinkNamedIfce(s1, dut)

    # Data plane links.
    net.addLink(
        dut, pktgen, bw=1000, delay="1ms", intfName1="vnf-in", intfName2="pktgen-out"
    )
    net.addLink(
        dut, pktgen, bw=1000, delay="1ms", intfName1="vnf-out", intfName2="pktgen-in"
    )
    pktgen.cmd("ip addr add 192.168.17.1/24 dev pktgen-out")
    pktgen.cmd("ip addr add 192.168.18.1/24 dev pktgen-in")
    dut.cmd("ip addr add 192.168.17.2/24 dev vnf-in")
    dut.cmd("ip addr add 192.168.18.2/24 dev vnf-out")

    # TODO: Deploy a chain of CNFs.
    cnfs = list()
    for n in range(1):
        cnf = mgr.addContainer(
            f"cnf{n}",
            "dut",
            f"ffpp:{FFPP_VER}",
            "/bin/bash",
            docker_args={
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
                    FFPP_DIR: {"bind": "/ffpp", "mode": "rw"},
                }
            },
        )
        cnfs.append(cnf)

    net.start()

    # Avoid looping
    pktgen.cmd("ip addr flush dev pktgen-s1")
    pktgen.cmd("ip link set pktgen-s1 down")
    dut.cmd("ip addr flush dev dut-s1")
    dut.cmd("ip link set dut-s1 down")

    pktgen.cmd("ping -c 5 192.168.17.2")
    pktgen.cmd("ping -c 5 192.168.18.2")

    duration = time.time() - start_ts
    print(f"Setup duration: {duration:.2f} seconds.")

    CLI(net)

    info("*** Stopping network\n")
    net.stop()
    mgr.stop()


if __name__ == "__main__":
    # Make sure xterm works inside sudo env.
    sudo_user = os.environ.get("SUDO_USER", None)
    if not sudo_user:
        print("Error: Run this script with sudo.")
        sys.exit(1)

    # TODO: Add this pre-setup in comnetsemu.
    if os.path.exists(f"/home/{sudo_user}/.Xauthority"):
        subprocess.run(
            f"xauth add $(xauth -f /home/{sudo_user}/.Xauthority list|tail -1)",
            check=True,
            shell=True,
        )
    # ISSUE: IPv6 support is currently not implemented in MuNF.
    subprocess.run("sysctl -w net.ipv6.conf.all.disable_ipv6=1", check=True, shell=True)

    parser = argparse.ArgumentParser(description="Emulation topology for MuNF system.")
    parser.add_argument(
        "--nano_cpus", type=float, default=8e8, help="Nano CPUs for VNF."
    )
    args = parser.parse_args()

    setLogLevel("info")
    testMuNF(nano_cpus=args.nano_cpus)

    subprocess.run("sysctl -w net.ipv6.conf.all.disable_ipv6=0", check=True, shell=True)
