#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Basic chain topology for benchmarking
"""

from comnetsemu.cli import CLI
from comnetsemu.net import Containernet
from mininet.link import TCLink
from mininet.log import info, setLogLevel
from mininet.node import Controller


def testTopo():

    net = Containernet(controller=Controller, link=TCLink)

    info("*** Adding controller\n")
    net.addController("c0")

    info("*** Adding hosts\n")
    client = net.addDockerHost(
        "client", dimage="lat_bm:latest", ip="10.0.0.100/24", cpuset_cpus="0"
    )
    server = net.addDockerHost(
        "server", dimage="lat_bm:latest", ip="10.0.0.200/24", cpuset_cpus="1"
    )

    info("*** Adding switch\n")
    s1 = net.addSwitch("s1")

    info("*** Creating links\n")
    net.addLink(s1, client, delay="100ms")
    net.addLink(s1, server, delay="100ms")

    info("*** Starting network\n")
    net.start()

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
