#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: Mininet topology for testing the computation offloading on SFC

Topo :
"""

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.util import dumpNodeConnections, custom
from mininet.log import setLogLevel, info
from mininet.cli import CLI
from mininet.term import makeTerms

from subprocess import check_output, check_call
from shlex import split
import sys


# Service function paths
SFP1 = ["dec1", "server"]


class TestTopo(Topo):

    def addLinkNamedIfce(self, src, dst, *args, **kwargs):
        """Add a link with named two interfaces"""
        self.addLink(src, dst,
                     intfName1="-".join((src, dst)),
                     intfName2="-".join((dst, src)),
                     * args, **kwargs
                     )

    def build(self):

        info("*** Add hosts\n")
        sw = self.addSwitch("s1")
        # TODO: Tune the CPU cycles of each host
        cam1 = self.addHost("cam1", ip="10.0.0.2", cpu=0.1)
        dec1 = self.addHost("dec1", ip="10.0.0.3", cpu=0.1)
        server = self.addHost("server", ip="10.0.0.10", cpu=0.1)
        car = self.addHost("car", ip="10.0.0.20", cpu=0.1)
        info("*** Add links\n")
        self.addLinkNamedIfce(cam1, sw, bw=10, delay="1ms",
                              loss=0, use_htb=False)
        self.addLinkNamedIfce(dec1, sw, bw=10, delay="1ms",
                              loss=0, use_htb=False)
        self.addLinkNamedIfce(
            server, sw, bw=10, delay="1ms", loss=0, use_htb=False)
        self.addLinkNamedIfce(car, sw, bw=10, delay="1ms",
                              loss=0, use_htb=False)


def get_ofport(ifce):
    """Get the openflow port based on iterface name

    :param ifce:
    """
    return check_output(
        split("sudo ovs-vsctl get Interface {} ofport".format(ifce)))


def build_scl(net, ovs, src, dst, next_hop, match=None):
    """Build the service classifier
       TODO: Add flow matches, currently all flows are redirected into the next
       hop
    """
    info("### Build service classifier, source:{}, next_hop:{}\n".format(src,
                                                                         next_hop))
    in_port = get_ofport("{}-{}".format(ovs, src))
    out_port = get_ofport("{}-{}".format(ovs, next_hop))
    ip_dst = net[dst].IP()
    check_output(
        split(
            "sudo ovs-ofctl add-flow {sw} \"nw_dst={ip_dst},icmp,in_port={in_port},actions=output={out_port}\"".format(
                **{"sw": ovs, "in_port": in_port, "out_port": out_port,
                   "ip_dst": ip_dst}
            )
        )
    )
    ret = check_output(split("sudo ovs-ofctl dump-flows {}".format(ovs)))
    info("### Flow table of the switch after adding SFP:\n")
    print(ret)


def build_sfp(net, ovs, sfp):
    """TODO: Build SFP by adding flows into OVS

    MARK: Since NSH support is still not mature in the OVS mainline
          Therefore, NSH is not used here ---> static OpenFlow rules.
    """
    ret = check_output(split("sudo ovs-ofctl dump-flows {}".format(ovs)))
    info("### Current flow table of the switch:\n")
    print(ret)
    info("### Add redirecting rules for sfc.\n")

    # MARK: Assume the first element of the sfp is the source and the last one
    # is the destination server. Elements in the middle are VNFs
    # The interface name is always switch_name-host_name


if __name__ == "__main__":
    setLogLevel("info")
    topo = TestTopo()
    net = Mininet(topo=topo, host=CPULimitedHost, link=TCLink,
                  autoStaticArp=True)
    net.start()
    dumpNodeConnections(net.hosts)
    info("*** Print host informations\n")
    for h in topo.hosts():
        print("{}: {}".format(h, net.get(h).IP()))

    build_scl(net, "s1", "cam1", "server", SFP1[0])
    # TODO: Build the service function path
    # build_sfp("s1", SFP1, net)
    if len(sys.argv) > 1:
        if sys.argv[1] == "-t":
            info("*** Open xterm for each host\n")
            host_objs = [net[h] for h in topo.hosts()]
            makeTerms(host_objs, term="xterm")
    cli = CLI(net)
    net.stop()
