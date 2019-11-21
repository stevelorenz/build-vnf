#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Basic chain topology for benchmarking DPDK forwarding applications.

MARK:  The API of ComNetEmu could change in the next release, benchmark script
       should be updated after the release.
"""

import argparse
import pathlib
import signal
import subprocess
from shlex import split
from subprocess import check_output

from comnetsemu.cli import CLI
from comnetsemu.net import Containernet
from mininet.link import TCLink
from mininet.log import info, setLogLevel
from mininet.node import Controller, OVSSwitch

ADD_Relay = True
TEST_NF = "l3fwd-power"
ENTER_CLI = False
DEBUG = False
# Name of the executable for CPU energy measurement. Should in $PATH
CPU_ENERGY_METER_BIN = "cpu-energy-meter"


def getOFPort(sw, ifce_name):
    """Get the openflow port based on iterface name"""
    return sw.vsctl(f"get Interface {ifce_name} ofport")


def run_l2fwd(relay):
    info("*** Run DPDK l2fwd sample application on the relay.\n")
    relay.cmd("cd $RTE_SDK/examples/l2fwd && make")
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


def run_l2fwd_power(relay):
    info("*** Run DPDK l2fwd-power application on the relay.\n")
    relay.cmd("cd /ffpp/modified_dpdk_samples/l2fwd-power/ && make")
    run_l2fwd_power_cmd = " ".join(
        [
            "./l2fwd -l 1 -m 256 --vdev=eth_af_packet0,iface=relay-s1",
            "--no-pci --single-file-segments",
            "-- -p 1 --no-mac-updating",
            "> /dev/null &",
        ]
    )
    print(f"The command to run l2fwd: {run_l2fwd_power_cmd}")
    ret = relay.cmd(
        f"cd /ffpp/modified_dpdk_samples/l2fwd-power/build && {run_l2fwd_power_cmd}"
    )
    print(f"The output of l2fwd-power app:\n{ret}")


# ISSUE: The DPDK built-in l3fwd application uses RSS by default, which is not
# supported by the veth vdev...
def run_l3fwd(relay):

    # info("*** Run DPDK l3fwd sample application on the relay.\n")
    # relay.cmd("cd $RTE_SDK/examples/l3fwd && make")
    # run_l3fwd_cmd = " ".join(
    #     [
    #         "./l3fwd -l 1 -m 256 --vdev=eth_af_packet0,iface=relay-s1",
    #         "--no-pci --single-file-segments",
    #         '-- -p 1 --config="(0,0,1)"',
    #         "> /dev/null &",
    #     ]
    # )
    # print(f"The command to run l3fwd: {run_l3fwd_cmd}")
    # ret = relay.cmd(f"cd $RTE_SDK/examples/l3fwd/build && {run_l3fwd_cmd}")
    # print(f"The output of l3fwd app:\n{ret}")

    print(
        "ISSUE: The DPDK built-in l3fwd application uses RSS by default,"
        "which is not supported by the veth vdev."
    )


DISPATCHER = {"l2fwd": run_l2fwd, "l2fwd-power": run_l2fwd_power, "l3fwd": run_l3fwd}


def run_udp_latency_test(server, client):
    info("*** Run Sockperf UDP under-load test for 10 seconds\n")
    server.cmd("sockperf server -i %s > /dev/null 2>&1 &" % server.IP())
    print(
        "[MARK] The average latency in the output is the estimated one-way"
        "path delay: The average RTT divided by two."
    )
    p = subprocess.Popen(
        args=[f"{CPU_ENERGY_METER_BIN}", "-r"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    client.cmdPrint(
        "sockperf under-load -i %s -t 10 --mps 50 --reply-every 1" % server.IP()
    )
    p.send_signal(signal.SIGINT)
    print("*** The output of CPU energy measurement.")
    print(p.stdout.read().decode())
    # TODO: Parse the measurement result and store it in a CSV file.


def run_benchmark():

    net = Containernet(
        controller=Controller, link=TCLink, switch=OVSSwitch, autoStaticArp=False
    )

    info("*** Adding controller\n")
    net.addController("c0")

    info("*** Adding switch\n")
    s1 = net.addSwitch("s1")
    # MARK: The relay should run on a different CPU core as the client and
    # server. To avoid cache misses of the VNF running on the relay.
    info("*** Adding client and server.\n")
    client = net.addDockerHost(
        "client",
        dimage="lat_bm:latest",
        ip="10.0.0.100/24",
        docker_args={"cpuset_cpus": "0"},
    )
    net.addLinkNamedIfce(s1, client, delay="100ms")

    server = net.addDockerHost(
        "server",
        dimage="lat_bm:latest",
        ip="10.0.0.200/24",
        docker_args={"cpuset_cpus": "0"},
    )
    net.addLinkNamedIfce(s1, server, delay="100ms")

    if ADD_Relay:
        info("*** Adding relay.\n")

        ffpp_dir = pathlib.Path.cwd().parent.absolute()
        # Need additional mounts to run DPDK application
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
                    "%s" % ffpp_dir: {"bind": "/ffpp", "mode": "rw"},
                },
            },
        )
        net.addLinkNamedIfce(s1, relay, delay="100ms")

    info("*** Starting network\n")
    net.start()
    net.pingAll()

    nodes = [n.name for n in net.hosts]
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

        info("*** Run DPDK helloworld\n")
        relay.cmd("cd $RTE_SDK/examples/helloworld && make")
        ret = relay.cmd("cd $RTE_SDK/examples/helloworld/build && ./helloworld")
        print(f"Output of helloworld app:\n{ret}")

        DISPATCHER[TEST_NF](relay)

    if DEBUG:
        flow_table = s1.dpctl("dump-flows")
        print(f"*** Current flow table of s1: \n {flow_table}")

    run_udp_latency_test(server, client)

    if ENTER_CLI:
        info("*** Enter CLI\n")
        info("Use help command to get CLI usages\n")
        CLI(net)

    info("*** Stopping network")
    net.stop()


if __name__ == "__main__":
    setLogLevel("info")

    parser = argparse.ArgumentParser(
        description="Basic chain topology for benchmarking DPDK forwarding applications."
    )
    parser.add_argument(
        "--relay_func",
        type=str,
        default="l2fwd",
        choices=["l2fwd", "l2fwd-power", "l3fwd"],
        help="The network function running on the relay. The default is l2fwd.",
    )
    parser.add_argument(
        "--no_relay",
        action="store_true",
        help="No relay in the middle. No OF rules are added. For debugging.",
    )
    parser.add_argument(
        "--cli", action="store_true", help="Enter ComNetEmu CLI after latency tests."
    )
    parser.add_argument(
        "--debug", action="store_true", help="Run in debug mode. e.g. print more log."
    )
    args = parser.parse_args()
    TEST_NF = args.relay_func
    ENTER_CLI = args.cli
    if args.debug:
        DEBUG = True
        setLogLevel("debug")
    if args.no_relay:
        print("*** No relay in the middle. No OF rules are added.")
        print("The value of relay_func argument is ignored.")
        ADD_Relay = False
    else:
        print("*** Relay is added with deployed network function: %s." % TEST_NF)

    run_benchmark()
