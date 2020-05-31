#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: ComNesEmu test topology for Trex traffic generator.
"""

import os
from shlex import split
from subprocess import check_output

from comnetsemu.cli import CLI
from comnetsemu.net import Containernet
from mininet.link import TCLink
from mininet.log import info, setLogLevel
from mininet.node import Controller

TREX_VERSION = "v2.74"


def get_ofport(ifce):
    """Get the openflow port based on iterface name

    :param ifce (str): Name of the interface
    """
    return check_output(split("ovs-vsctl get Interface {} ofport".format(ifce))).decode(
        "utf-8"
    )


def disable_cksum_offload(ifces):
    """Disable RX/TX checksum offloading"""
    for ifce in ifces:
        check_output(split("sudo ethtool --offload %s rx off tx off" % ifce))


def testTopo():

    # xterms=True, spawn xterms for all nodes after net.start()
    net = Containernet(controller=Controller, link=TCLink, xterms=True)

    info("*** Adding controller\n")
    net.addController("c0")

    info("*** Adding hosts\n")
    h1 = net.addDockerHost(
        "h1",
        dimage="trex:latest",
        ip="10.0.0.1/24",
        docker_args={
            # Run Trex daemon server in background.
            "cpuset_cpus": "0",
            "hostname": "h1",
            "volumes": {
                os.getcwd(): {"bind": f"/trex/{TREX_VERSION}/local", "mode": "rw"},
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
    h2 = net.addDockerHost(
        "h2",
        dimage="trex:latest",
        ip="10.0.0.3/24",
        docker_args={"cpuset_cpus": "0", "hostname": "h2"},
    )
    info("*** Adding switch\n")
    net.addSwitch("s1")

    # Trex currently requires that each host has minimal two interfaces.
    # Each host has two interfaces here, one for ingress traffic and one for
    # egress traffic.
    info("*** Creating links\n")
    net.addLink(
        "h1",
        "s1",
        intfName1="-".join(("h1-ingress", "s1")),
        intfName2="-".join(("s1", "h1-ingress")),
        bw=10,
        delay="100ms",
    )
    # Mininet only gives the IP to the first added interface.
    net.addLink(
        "h1",
        "s1",
        intfName1="-".join(("h1-egress", "s1")),
        intfName2="-".join(("s1", "h1-egress")),
        bw=10,
        delay="100ms",
    )
    net.addLink(
        "h2",
        "s1",
        intfName1="-".join(("h2-ingress", "s1")),
        intfName2="-".join(("s1", "h2-ingress")),
        bw=10,
        delay="100ms",
    )
    net.addLink(
        "h2",
        "s1",
        intfName1="-".join(("h2-egress", "s1")),
        intfName2="-".join(("s1", "h2-egress")),
        bw=10,
        delay="100ms",
    )

    info("*** Starting network\n")
    net.start()
    h1.cmd("ip addr add 10.0.0.2/24 dev h1-egress-s1")
    h2.cmd("ip addr add 10.0.0.4/24 dev h2-egress-s1")

    sw_ifces = [f"s1-{h}-{d}" for h in ["h1", "h2"] for d in ["ingress", "egress"]]
    disable_cksum_offload(sw_ifces)

    print("Add static ARPs on h1 and h2")

    print("Add Openflow rules.")
    h1_egress_port = get_ofport("s1-h1-egress")
    h2_ingress_port = get_ofport("s1-h2-ingress")
    check_output(
        split(
            f'ovs-ofctl add-flow s1 "udp,in_port={h1_egress_port},actions=output={h2_ingress_port}"'
        )
    )

    info("*** Launch Trex server in stateless mode.\n")
    ret = h1.cmd("./t-rex-64 -i --stl &")
    print("Output: " + ret)

    info("*** Enter CLI\n")
    CLI(net)
    info("Use help command to get CLI usages\n")

    info("*** Stopping network")
    net.stop()


if __name__ == "__main__":
    setLogLevel("info")
    testTopo()
