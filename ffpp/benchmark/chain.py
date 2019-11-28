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
import multiprocessing
import signal
import subprocess
import sys
import time
from shlex import split
from subprocess import check_output

from comnetsemu.cli import CLI
from comnetsemu.net import Containernet
from mininet.link import TCLink
from mininet.log import info, setLogLevel
from mininet.node import Controller, OVSSwitch

# Parameters for latency test running on the client.
LAT_TEST_PARAS = {
    "client_protocols": ["udp", "tcp"],
    "client_mps_list": [0, 50],
    # Following parameters are ignored if enable_energy_monitor == False
    "enable_energy_monitor": False,
    "enable_powertop": True,
    "enable_cpu_energy_meter": False,
    "cpu_energy_meter_bin": "cpu-energy-meter",
    "test_duration_sec": 30,
}


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
            "./l2fwd-power -l 1 -m 256 --vdev=eth_af_packet0,iface=relay-s1",
            "--no-pci --single-file-segments",
            "-- -p 1 --no-mac-updating",
            "> /dev/null &",
        ]
    )
    print(f"The command to run l2fwd-power: {run_l2fwd_power_cmd}")
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

FFPP_DIR = pathlib.Path.cwd().parent.absolute()


def setup_server(server, proto="udp"):
    proto_option = ""
    if proto == "tcp":
        proto_option = "--tcp"

    info(f"*** Run Sockperf server on server node. Proto:{proto}\n")
    server.cmd(f"sockperf server {proto_option} -i {server.IP()} > /dev/null 2>&1 &")


def run_latency_test(server, client, proto="udp", mps=0):
    test_duration_sec = LAT_TEST_PARAS["test_duration_sec"]
    proto_option = ""
    if proto == "tcp":
        proto_option = "--tcp"

    if LAT_TEST_PARAS["enable_energy_monitor"]:
        if LAT_TEST_PARAS["enable_cpu_energy_meter"]:
            print("* Run cpu_energy_meter in raw mode.")
            meter_bin = LAT_TEST_PARAS["cpu_energy_meter_bin"]
            p = subprocess.Popen(
                args=[f"{meter_bin}", "-r"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
        if LAT_TEST_PARAS["enable_powertop"]:
            print("* Run powertop with CSV output.")
            csv_name = f"powertop_stats_proto_{proto}_mps_{mps}.csv"
            subprocess.run(
                split(f"powertop --csv={csv_name} -t {test_duration_sec + 3} &"),
                check=True,
                stdout=subprocess.DEVNULL,
            )
            time.sleep(3)
    else:
        print("* Energy monitoring is disabled.")

    if mps != 0:
        print(f"Run sockperf under-load test with l4 protocol: {proto} and mps: {mps}")
        print(
            "[MARK] The average latency in the output is the estimated one-way"
            "path delay: The average RTT divided by two."
        )
        client.cmdPrint(
            "sockperf under-load {} -i {} -t {} --mps {} --reply-every 1".format(
                proto_option, server.IP(), test_duration_sec, mps
            )
        )
        # TODO: Parse output of under-load test and store the percentile 50 and
        # 99.999 latency for further processing
    else:
        print(f"No traffic is sent, wait {test_duration_sec} seconds.")
        time.sleep(test_duration_sec)

    if LAT_TEST_PARAS["enable_energy_monitor"]:
        if LAT_TEST_PARAS["enable_cpu_energy_meter"]:
            p.send_signal(signal.SIGINT)
            print("*** The output of CPU energy measurement.")
            energy_stats = p.stdout.read().decode()
            print(energy_stats)
            with open(f"./cpu_energy_{TEST_NF}.csv", "a+") as f:
                f.write(energy_stats)


def run_benchmark(proto):
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
        docker_args={
            "cpuset_cpus": "0",
            "volumes": {"%s" % FFPP_DIR: {"bind": "/ffpp", "mode": "ro"}},
        },
    )
    net.addLinkNamedIfce(s1, client, delay="100ms")

    server = net.addDockerHost(
        "server",
        dimage="lat_bm:latest",
        ip="10.0.0.200/24",
        docker_args={"cpuset_cpus": "0"},
    )
    net.addLinkNamedIfce(s1, server, delay="100ms")

    if ADD_RELAY:
        cpus_relay = "1"
        if TEST_NF == "l2fwd-power":
            print(
                "*** [INFO] l2fwd-power application require at least one master and one slave core.\n"
                "The master handles timers and slave core handles forwarding task."
            )
            cpus_relay = "0,1"
        info("*** Adding relay.\n")
        # Need additional mounts to run DPDK application
        # MARK: Just used for development, never use this in production container
        # setup.
        relay = net.addDockerHost(
            "relay",
            dimage="ffpp:latest",
            ip="10.0.0.101/24",
            docker_args={
                "cpuset_cpus": cpus_relay,
                "nano_cpus": int(1.0 * 1e9),
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
                    "%s" % FFPP_DIR: {"bind": "/ffpp", "mode": "rw"},
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

    if ADD_RELAY:
        info("*** Add OpenFlow rules for traffic redirection.\n")
        peer_map = {"client": "relay", "relay": "server", "server": "client"}
        for p in ["udp", "tcp"]:
            for peer in peer_map.keys():
                check_output(
                    split(
                        'ovs-ofctl add-flow s1 "{},in_port={},actions=output={}"'.format(
                            p, node_portnum_map[peer], node_portnum_map[peer_map[peer]],
                        )
                    )
                )

        if DEBUG:
            flow_table = s1.dpctl("dump-flows")
            print(f"*** Current flow table of s1: \n {flow_table}")

        info("*** Run DPDK helloworld\n")
        relay.cmd("cd $RTE_SDK/examples/helloworld && make")
        ret = relay.cmd("cd $RTE_SDK/examples/helloworld/build && ./helloworld")
        print(f"Output of helloworld app:\n{ret}")

        DISPATCHER[TEST_NF](relay)

        server.cmd("pkill sockperf")
        setup_server(server, proto)
        for mps in LAT_TEST_PARAS["client_mps_list"]:
            run_latency_test(server, client, proto, mps)
            time.sleep(3)

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
        choices=["l2fwd", "l2fwd-power"],
        help="The network function running on the relay. The default is l2fwd.",
    )
    parser.add_argument(
        "--cli", action="store_true", help="Enter ComNetEmu CLI after latency tests."
    )
    parser.add_argument(
        "--debug", action="store_true", help="Run in debug mode. e.g. print more log."
    )
    parser.add_argument(
        "--no_relay",
        action="store_true",
        help="No relay in the middle. No OF rules are added. For debugging.",
    )
    parser.add_argument(
        "--enable_energy_monitor",
        action="store_true",
        help="Enable energy monitoring for latency tests.",
    )

    args = parser.parse_args()
    TEST_NF = args.relay_func
    ENTER_CLI = args.cli
    ADD_RELAY = True
    DEBUG = False

    if args.debug:
        DEBUG = True
        setLogLevel("debug")

    if args.no_relay:
        print("*** No relay in the middle. No OF rules are added.")
        print("The value of relay_func argument is ignored.")
        ADD_RELAY = False
    else:
        print("*** Relay is added with deployed network function: %s." % TEST_NF)

    if args.enable_energy_monitor:
        print("*** Enable energy monitoring for latency tests")
        LAT_TEST_PARAS["enable_energy_monitor"] = True

    if multiprocessing.cpu_count() < 2:
        print("[ERROR]: This benchmark requires minimal 2 available CPU cores.")
        sys.exit(1)

    for proto in LAT_TEST_PARAS["client_protocols"]:
        run_benchmark(proto)
