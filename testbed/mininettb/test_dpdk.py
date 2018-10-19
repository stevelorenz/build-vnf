#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: Test if DPDK can work properly on Mininet hosts

Topo : Two hosts are connected to the single switch
"""

from mininet.cli import CLI
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel, info


class TestTopo(Topo):

    def build(self):
        switch = self.addSwitch("s1")

        for h in range(2):
            host = self.addHost("h{}".format(h+1))
            self.addLink(host, switch, bw=10, delay="1ms", loss=0)


if __name__ == "__main__":
    setLogLevel("info")
    topo = TestTopo()
    net = Mininet(
        topo=topo,
        host=CPULimitedHost, link=TCLink,
        autoStaticArp=True
    )
    info("Start network and run simple CLI...")
    net.start()
    CLI(net)
    net.stop()
