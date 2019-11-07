#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Basic chain topology for benchmarking

MARK:  The API of ComNetEmu could change in the next release, benchmark script
       should be updated after the release.
"""

import sys
from shlex import split
from subprocess import check_output

from comnetsemu.cli import CLI
from comnetsemu.net import Containernet
from mininet.link import TCLink
from mininet.log import info, setLogLevel
from mininet.node import Controller, OVSSwitch


ADD_Relay = False


def getOFPort(sw, ifce_name):
    """Get the openflow port based on iterface name"""
    return sw.vsctl(f"get Interface {ifce_name} ofport")


def testTopo():

    net = Containernet(
        controller=Controller, link=TCLink, switch=OVSSwitch, autoStaticArp=True
    )

    info("*** Adding controller\n")
    net.addController("c0")

    # MARK: The relay should run on a different CPU core as the client and
    # server. To avoid cache misses of the VNF running on the relay.
    info("*** Adding hosts\n")
    client = net.addDockerHost(
        "client",
        dimage="lat_bm:latest",
        ip="10.0.0.100/24",
        docker_args={"cpuset_cpus": "0"},
    )

    # Need addtional mounts to run DPDK application
    # MARK: Just used for development, never use this in production container
    # setup.
    relay = net.addDockerHost(
        "relay",
        dimage="ffpp:latest",
        ip="10.0.0.101/24",
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

    server = net.addDockerHost(
        "server",
        dimage="lat_bm:latest",
        ip="10.0.0.200/24",
        docker_args={"cpuset_cpus": "0"},
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

    if ADD_Relay:
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
        check_output(
            split(
                'ovs-ofctl add-flow s1 "udp,in_port={},actions=output={}"'.format(
                    node_portnum_map["server"], node_portnum_map["client"]
                )
            )
        )
        flow_table = s1.dpctl("dump-flows")
        print(f"*** Current flow table: \n {flow_table}")

        info("*** Run DPDK helloworld\n")
        relay.cmd("cd $RTE_SDK/examples/helloworld && make")
        ret = relay.cmd("cd $RTE_SDK/examples/helloworld/build && ./helloworld")
        print(f"Output of helloworld app:\n{ret}")

        info("*** Run DPDK l2fwd program on the relay\n")
        relay.cmd("cd $RTE_SDK/examples/l2fwd&& make")
        run_l2fwd_cmd = " ".join(
            [
                "./l2fwd -l 1 -m 256 --vdev=eth_af_packet0,iface=relay-s1",
                "--no-pci --single-file-segments",
                "-- -p 1 --no-mac-updating",
                "> /dev/null &",
            ]
        )
        print(f"The command to run l2fwd: {run_l2fwd_cmd}")
        ret = relay.cmd(f"cd $RTE_SDK/examples/l2fwd/build && {run_l2fwd_cmd}")
        print(f"The output of l2fwd app:\n{ret}")

    info("*** Run Sockperf UDP under-load test for 10 seconds\n")
    server.cmd("sockperf server -i %s > /dev/null 2>&1 &" % server.IP())
    client.cmdPrint(
        "sockperf under-load -i %s -t 10 --mps 50 --reply-every 1" % server.IP()
    )

    info("*** Enter CLI\n")
    info("Use help command to get CLI usages\n")
    CLI(net)

    info("*** Stopping network")
    net.stop()


def usage():
    print("Usage: sudo python3 chain.py [Option]")
    print("Option:")
    print(
        "\t- (default without any option): Client and server are directly connected, no relay in the middle."
    )
    print(
        "\t- add_relay: Add a relay in the middle running DPDK l2fwd. "
        "Traffic is redirected via Openflow rules."
    )
    sys.exit(0)


if __name__ == "__main__":
    setLogLevel("info")
    if len(sys.argv) > 1:
        if sys.argv[1] == "-h" or sys.argv[1] == "--help":
            usage()
        elif sys.argv[1] == "add_relay":
            print("Add a relay in the middle running DPDK l2fwd.")
            ADD_Relay = True
        else:
            print("Invalid option!")
            usage()
    testTopo()
