#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Basic dumbbell topology to test COIN-DL
"""

import argparse
import json
import os
import shlex
import subprocess
import sys
import time

from comnetsemu.cli import CLI
from comnetsemu.net import Containernet
from mininet.link import TCLink
from mininet.log import info, setLogLevel
from mininet.node import Controller, RemoteController
from mininet.term import makeTerm

CURRENT_DIR = os.path.abspath(os.path.curdir)
DIMAGE = "coin_dl:0.1.0"

SFC_PORT = 9999
NO_SFC_PORT = SFC_PORT + 1
CONTROL_PORT = NO_SFC_PORT + 1


def check_env():
    cpu_count = os.cpu_count()
    if cpu_count < 3:
        raise RuntimeError("Requires at least 3 (v)CPUs")


def create_dumbbell():
    """Create a single dumbbell topology with the given number of hosts on one side"""
    num_hosts = 2
    net = Containernet(
        controller=Controller,
        link=TCLink,
        xterms=False,
    )
    topo_params = {
        "sfc_port": SFC_PORT,
        "no_sfc_port": NO_SFC_PORT,
        "control_port": CONTROL_PORT,
    }
    nodes = {}

    info("*** Adding controller, listening on port 6653, use OpenFlow 1.3\n")
    c1 = net.addController(
        "c1", controller=RemoteController, port=6653, protocols="OpenFlow13"
    )
    topo_params.update({"controllers": [{"name": "c1", "port": 6653}]})

    info(f"*** Adding {num_hosts} clients and {num_hosts} servers\n")
    # Boilerplate code...
    clients = []
    servers = []
    topo_params["clients"] = {}
    topo_params["servers"] = {}
    # TODO: Play with cpuset_cpus if measurement results are not good enough
    for i in range(1, num_hosts + 1):
        client_ip = f"10.0.1.{10+i}/16"
        server_ip = f"10.0.2.{10+i}/16"
        client = net.addDockerHost(
            f"client{i}",
            dimage=f"{DIMAGE}",
            ip=client_ip,
            docker_args={
                "cpuset_cpus": "0",
                "nano_cpus": int(1e9),
                "hostname": f"client{i}",
                "working_dir": "/coin_dl",
                "volumes": {
                    CURRENT_DIR: {"bind": "/coin_dl/share", "mode": "rw"},
                },
            },
        )
        server = net.addDockerHost(
            f"server{i}",
            dimage=f"{DIMAGE}",
            ip=server_ip,
            docker_args={
                "cpuset_cpus": "1",
                "nano_cpus": int(1e9),
                "hostname": f"server{i}",
                "working_dir": "/coin_dl",
                "volumes": {
                    CURRENT_DIR: {"bind": "/coin_dl/share", "mode": "rw"},
                },
            },
        )
        clients.append(client)
        topo_params["clients"][client.name] = {
            "ip": client_ip,
        }
        servers.append(server)
        topo_params["servers"][server.name] = {
            "ip": server_ip,
        }

    info("*** Adding two switches and two VNF nodes\n")
    s1 = net.addSwitch("s1", protocols="OpenFlow13")
    s2 = net.addSwitch("s2", protocols="OpenFlow13")
    switches = [s1, s2]

    DPDK_VOLUME = {
        "/sys/kernel/mm/hugepages": {
            "bind": "/sys/kernel/mm/hugepages",
            "mode": "rw",
        },
        "/dev": {"bind": "/dev", "mode": "rw"},
        CURRENT_DIR: {"bind": "/coin_dl/share", "mode": "rw"},
    }

    vnfs = []
    for i in range(1, len(switches) + 1):
        vnf = net.addDockerHost(
            f"vnf{i}",
            dimage=f"{DIMAGE}",
            ip=f"10.0.3.{10+i}/16",
            docker_args={
                "cpuset_cpus": "3",
                "nano_cpus": int(9e8 / 2),
                "hostname": f"vnf{i}",
                "volumes": DPDK_VOLUME,
                "working_dir": "/coin_dl",
            },
        )
        vnfs.append(vnf)

    # TODO: Pick up the suitable parameters here.
    BW_MAX = 1000  # 1Gb/sec
    BW_BOTTLENECK = 100  # Mbits/s
    DELAY_BOTTLENECK_MS = 10
    DELAY_BOTTLENECK_MS_STR = str(DELAY_BOTTLENECK_MS) + "ms"
    BDP = ((BW_BOTTLENECK * 10 ** 6) * (DELAY_BOTTLENECK_MS * 10 ** -3)) / 8.0
    info(f"\n*** The BDP of the bottleneck link is {BDP:.2f} B\n")
    topo_params.update(
        {
            "bottleneck": {
                "bandwidth_mbps": BW_BOTTLENECK,
                "delay_ms": DELAY_BOTTLENECK_MS,
                "BDP_B": BDP,
            }
        }
    )

    info("*** Creating links\n")
    for client, server in zip(clients, servers):
        # TODO (Zuo): Check if max_queue_size need to be added here
        net.addLinkNamedIfce(client, s1, bw=BW_MAX, delay=DELAY_BOTTLENECK_MS_STR)
        net.addLinkNamedIfce(s2, server, bw=BW_MAX, delay=DELAY_BOTTLENECK_MS_STR)
    # Add bottleneck link
    net.addLinkNamedIfce(s1, s2, bw=BW_BOTTLENECK, delay=DELAY_BOTTLENECK_MS_STR)

    for switch, vnf in zip(switches, vnfs):
        # TODO (Zuo): Check if max_queue_size need to be added here, to remove additional queuing latency of VNF data interfaces
        net.addLinkNamedIfce(switch, vnf, bw=BW_MAX, delay="0ms")

    with open("./dumbbell.json", "w+", encoding="ascii") as f:
        json.dump(topo_params, f, sort_keys=True, indent=2)

    nodes["clients"] = clients
    nodes["servers"] = servers
    nodes["switches"] = switches
    nodes["vnfs"] = vnfs
    nodes["controllers"] = [c1]

    return (net, nodes, topo_params)


def start_controllers(controllers):
    info("Starting controllers\n")
    assert len(controllers) == 1
    c1 = controllers[0]
    makeTerm(c1, cmd="ryu-manager ./dumbbell_controller.py")


def disable_checksum_offload(nodes):
    info("*** Disable RX and TX checksum offloading on all hosts\n")
    sw = nodes["switches"][0]
    for client in nodes["clients"]:
        client.cmd(f"ethtool --offload {client.name}-{sw.name} rx off")
        client.cmd(f"ethtool --offload {client.name}-{sw.name} tx off")

    sw = nodes["switches"][-1]
    for server in nodes["servers"]:
        server.cmd(f"ethtool --offload {server.name}-{sw.name} rx off")
        server.cmd(f"ethtool --offload {server.name}-{sw.name} tx off")

    for vnf, switch in zip(nodes["vnfs"], nodes["switches"]):
        vnf.cmd(f"ethtool --offload {vnf.name}-{switch.name} rx off")
        vnf.cmd(f"ethtool --offload {vnf.name}-{switch.name} tx off")


def deploy_servers(servers):
    info("*** Deploying all servers \n")
    # Use to measure base line bandwidth and latency.
    for s in servers:
        s.cmd(f"iperf3 -s --daemon")
        s.cmd(f"sockperf server --daemonize")


def deploy_vnfs(vnfs, vnf_mode):
    assert len(vnfs) == 2
    # Avoid the ARP storm because the ARP response is not implemented in the VNF
    # VNF nodes work currently on layer 2
    info("*** Flushing the IP address of VNF's data plane interface.\n")
    for idx, v in enumerate(vnfs):
        v.cmd(f"ip addr flush vnf{idx+1}-s{idx+1}")

    if vnf_mode == "":
        info("*** VNF mode is empty. No VNFs are deployed.\n")
        info("Check the -m/--mode option in CLI for available VNF modes.\n")
        return
    elif vnf_mode == "store_forward":
        info("*** Deploying all VNFs with mode store_forward\n")
        for _, v in enumerate(vnfs):
            v.cmd("./build/coin_dl --mode store_forward > /var/log/coin_dl.log 2>&1 &")


def run_test_sockperf(nodes):
    info("*** Running test: sockperf\n")

    info("*** Run single client sockperf with under-load mode\n")

    info("*** Run multi-clients sockperf with under-load mode\n")


def run_store_forward(nodes):
    info("*** Running test: store and forward\n")


def run_tests(nodes: list, tests: list):
    test_func_map = {"sockperf": run_test_sockperf, "store_forward": run_store_forward}
    for test in tests:
        if test in test_func_map:
            test_func_map[test](nodes)
        else:
            raise RuntimeError(f"Function for test: {test} is not defined!")


def parse_tests(tests_str):
    if tests_str == "":
        return []
    tests = tests_str.strip().split(",")
    tests = [t.strip() for t in tests if t != ""]
    assert len(tests) > 0
    return tests


def main():
    if os.geteuid() != 0:
        print("Run this script with sudo.", file=sys.stderr)
        sys.exit(1)

    parser = argparse.ArgumentParser(description="Dumbbell topology to test COIN-DL")
    parser.add_argument(
        "-m",
        "--vnf_mode",
        type=str,
        default="",
        choices=["", "store_forward", "compute_forward"],
        help="Working mode of all VNF(s)",
    )
    parser.add_argument(
        "--tests",
        type=str,
        default="",
        help="Test names separated by commas. Empty string means no tests. Example: sockperf,store_forward",
    )
    parser.add_argument("--dev", action="store_true", help="Run in development mode")
    args = parser.parse_args()

    check_env()
    setLogLevel("info")

    tests = parse_tests(args.tests)
    if not tests:
        info("*** No tests are going to be performed.\n")

    # IPv6 is currently not used, disable it.
    subprocess.run(
        shlex.split("sysctl -w net.ipv6.conf.all.disable_ipv6=1"),
        check=True,
    )

    net = None
    nodes = None
    try:
        net, nodes, _ = create_dumbbell()
        # Use the minimal dumbbell as the start
        assert len(nodes["vnfs"]) == 2
        if net:
            net.start()
        else:
            raise RuntimeError("Failed to create the dumbbell topology")
        disable_checksum_offload(nodes)
        start_controllers(nodes["controllers"])
        time.sleep(3)
        net.pingAll()
        deploy_vnfs(nodes["vnfs"], args.vnf_mode)
        deploy_servers(nodes["servers"])
        run_tests(nodes, tests)
        if args.dev:
            CLI(net)
    finally:
        if net:
            info("*** Stopping network\n")
            net.stop()
        info("*** Killing Ryu manager\n")
        subprocess.run(shlex.split("sudo killall ryu-manager"), check=False)
        info("*** Trying to clean up ComNetsEmu\n")
        subprocess.run(
            shlex.split("sudo ce -c"),
            check=True,
            capture_output=False,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        subprocess.run(
            shlex.split("sysctl -w net.ipv6.conf.all.disable_ipv6=0"),
            check=True,
        )


if __name__ == "__main__":
    main()
