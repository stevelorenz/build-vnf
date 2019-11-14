#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Test topology for FFPP's functional tests.

Pktgen (Packet generator) <---> DUT (Device Under Test)

"""

import sys
from shlex import split
from subprocess import check_output

from comnetsemu.cli import CLI
from comnetsemu.net import Containernet
from mininet.link import TCLink
from mininet.log import info, setLogLevel
from mininet.node import Controller, OVSSwitch


def getOFPort(sw, ifce_name):
    """Get the openflow port based on iterface name"""
    return sw.vsctl(f"get Interface {ifce_name} ofport")


def testTopo():

    net = Containernet(
        controller=Controller, link=TCLink, switch=OVSSwitch, autoStaticArp=True
    )

    info("*** Adding controller\n")
    net.addController("c0")

    info("*** Adding hosts\n")
    pktgen = net.addDockerHost(
        "pktgen",
        dimage="ffpp_test:latest",
        ip="10.0.0.100/24",
        docker_args={"cpuset_cpus": "0"},
    )

    dut = net.addDockerHost(
        "dut",
        dimage="ffpp:latest",
        ip="10.0.0.200/24",
        docker_args={
            "cpuset_cpus": "1",
            "nano_cpus": int(0.6 * 1e9),
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
            },
        },
    )

    info("*** Adding switch\n")
    s1 = net.addSwitch("s1")

    info("*** Creating links\n")
    net.addLinkNamedIfce(s1, pktgen)
    net.addLinkNamedIfce(s1, dut)

    info("*** Starting network\n")
    net.start()
    net.pingAll()

    info("*** Enter CLI\n")
    info("Use help command to get CLI usages\n")
    CLI(net)

    info("*** Stopping network")
    net.stop()


if __name__ == "__main__":
    setLogLevel("info")
    testTopo()
