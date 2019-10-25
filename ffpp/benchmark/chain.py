#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Basic chain topology for benchmarking

MARK:  The API of ComNetEmu could change in the next release, benchmark script
       should be updated after the release.
"""

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

    net = Containernet(controller=Controller, link=TCLink, switch=OVSSwitch)

    info("*** Adding controller\n")
    net.addController("c0")

    # MARK: The relay should run on a different CPU core as the client and
    # server. To avoid cache misses of the VNF running on the relay.
    info("*** Adding hosts\n")
    client = net.addDockerHost(
        "client", dimage="lat_bm:latest", ip="10.0.0.100/24", cpuset_cpus="0"
    )

    relay = net.addDockerHost(
        "relay", dimage="ffpp:latest", ip="10.0.0.101/24", cpuset_cpus="1"
    )

    server = net.addDockerHost(
        "server", dimage="lat_bm:latest", ip="10.0.0.200/24", cpuset_cpus="0"
    )

    info("*** Adding switch\n")
    s1 = net.addSwitch("s1")

    info("*** Creating links\n")
    net.addLinkNamedIfce(s1, client, delay="100ms")
    net.addLinkNamedIfce(s1, relay, delay="100ms")
    net.addLinkNamedIfce(s1, server, delay="100ms")

    info("*** Starting network\n")
    net.start()

    nodes = ["client", "relay", "server"]
    sw_ifaces = [f"s1-{n}" for n in nodes]

    info("*** Disable kernel IP checksum offloading.\n")
    for iface in sw_ifaces:
        check_output(split(f"ethtool --offload {iface} rx off tx off"))

    node_portnum_map = {n: getOFPort(s1, f"s1-{n}") for n in nodes}
    info("*** Add OpenFlow rules for traffic redirection.\n")
    check_output(
        split(
            'ovs-ofctl add-flow s1 "udp,in_port={},actions=output={}"'.format(
                node_portnum_map["client"], node_portnum_map["relay"]
            )
        )
    )
    check_output(
        split(
            'ovs-ofctl add-flow s1 "udp,in_port={},actions=output={}"'.format(
                node_portnum_map["relay"], node_portnum_map["server"]
            )
        )
    )
    flow_table = s1.dpctl("dump-flows")
    print(f"*** Current flow table: \n {flow_table}")

    info("*** Run DPDK l2fwd program on the relay\n")

    info("*** Run Sockperf UDP ping-pong test for 10 seconds\n")
    server.cmd("sockperf server -i %s > /dev/null 2>&1 &" % server.IP())
    client.cmdPrint("sockperf ping-pong -i %s -t 10" % server.IP())

    info("*** Enter CLI\n")
    info("Use help command to get CLI usages\n")
    CLI(net)

    info("*** Stopping network")
    net.stop()


if __name__ == "__main__":
    setLogLevel("info")
    testTopo()
