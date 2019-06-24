#! /usr/bin/env python2
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: Mininet topology for testing the computation offloading on SFC

Topo :
"""

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost, OVSSwitch, RemoteController
from mininet.link import TCLink
from mininet.util import dumpNodeConnections, custom
from mininet.log import setLogLevel, info, warn
from mininet.cli import CLI
from mininet.term import makeTerms

from subprocess import check_output, check_call
from shlex import split
import sys
import argparse

# Service function paths
SFP1 = ["dec1", "server"]


class TestTopo(Topo):
    """MARK: Containernet does not implement addDocker method in topo"""

    def addLinkNamedIfce(self, src, dst, *args, **kwargs):
        """Add a link with named two interfaces"""
        self.addLink(src, dst,
                     intfName1="-".join((src, dst)),
                     intfName2="-".join((dst, src)),
                     * args, **kwargs
                     )

    def build(self):
        """
        TODO:
        - Build topo with descriptors in ./vnffgd.yaml
        - Extend this to multipath with multiple cameras
        """

        info("*** Add hosts\n")
        sw = self.addSwitch("s1")
        # TODO: Tune the CPU cycles of each host
        cam1 = self.addHost("cam1", ip="10.0.0.2",
                            mac="00:00:00:00:00:02", cpu=0.1)

        # Service CLassifier
        scl = self.addHost("scl", ip="10.0.0.100",
                           mac="00:00:00:00:01:10", cpu=0.1)

        dec1 = self.addHost("dec1", ip="10.0.0.101",
                            mac="00:00:00:00:01:01", cpu=0.1)

        server = self.addHost("server", ip="10.0.0.200",
                              mac="00:00:00:00:02:00", cpu=0.1)

        info("*** Add links\n")
        self.addLinkNamedIfce(scl, sw, bw=10, delay="1ms",
                              loss=0, use_htb=False)
        self.addLinkNamedIfce(cam1, sw, bw=10, delay="1ms",
                              loss=0, use_htb=False)
        self.addLinkNamedIfce(dec1, sw, bw=10, delay="1ms",
                              loss=0, use_htb=False)
        self.addLinkNamedIfce(
            server, sw, bw=10, delay="1ms", loss=0, use_htb=False)


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
    parser = argparse.ArgumentParser(
        description="Mininet topology for testing the computation offloading on SFC")
    parser.add_argument("--term", action="store_true",
                        help="Open xterm for each host")
    parser.add_argument("--debug", action="store_true",
                        help="Enable debug mode")
    parser.add_argument("--remote_controller", action="store_true",
                        help="Connect to the remote controller")
    parser.add_argument("--use_ofctl", action="store_true",
                        help="Use ofctl to add flows locally")

    args = parser.parse_args()

    if args.debug:
        setLogLevel("debug")

    topo = TestTopo()
    # MARK: autoStaticArp MUST be true since currently ARP protocol is not
    # implemented in raw socket based SCL and VNFs
    net = Mininet(topo=topo, host=CPULimitedHost, link=TCLink,
                  autoStaticArp=True, build=False)

    if args.remote_controller:
        info("*** Use remote controller\n")
        warn("*** The controller program should already run and listen on port 6653\n")
        c = RemoteController("ryu", ip="127.0.0.1", port=6653)
        net.addController(c)
        net.build()
        net.start()
    else:
        net.build()
        net.start()
        dumpNodeConnections(net.hosts)
        if args.use_ofctl:
            info("*** Add flows by using ovs-ofctl locally\n")
            build_scl(net, "s1", "cam1", "server", SFP1[0])
            # TODO: Build the service function path
            # build_sfp("s1", SFP1, net)

    info("*** Print host informations\n")
    for h in topo.hosts():
        print("{}: {}".format(h, net.get(h).IP()))

    if args.term:
        host_objs = [net[h] for h in topo.hosts()]
        makeTerms(host_objs, term="xterm")

    cli = CLI(net)
    net.stop()
