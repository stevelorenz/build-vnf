#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Simple topology to test running DPDK-based network functions as
       micro-services.
"""

import os
import time

from comnetsemu.cli import CLI, spawnXtermDocker
from comnetsemu.net import Containernet, VNFManager
from mininet.log import info, setLogLevel
from mininet.link import TCLink
from mininet.node import Controller

APPCONTAINER_TMP_PATH = "/tmp/comnetsemu/appcontainer"
RTE_CONFIG = os.path.join(APPCONTAINER_TMP_PATH, "rte")
os.makedirs(RTE_CONFIG, exist_ok=True)

CWD = os.getcwd()
FFPP_VOLS = {
    "/sys/bus/pci/drivers": {"bind": "/sys/bus/pci/drivers", "mode": "rw"},
    "/sys/kernel/mm/hugepages": {"bind": "/sys/kernel/mm/hugepages", "mode": "rw",},
    "/sys/devices/system/node": {"bind": "/sys/devices/system/node", "mode": "rw",},
    "/dev": {"bind": "/dev", "mode": "rw"},
    f"{CWD}": {"bind": "/ffpp_app", "mode": "rw"},
    f"{RTE_CONFIG}": {"bind": "/var/run/dpdk/rte/", "mode": "rw"},
}

if __name__ == "__main__":

    setLogLevel("info")

    net = Containernet(controller=Controller, link=TCLink, xterms=False)
    mgr = VNFManager(net)

    info("*** Add controller\n")
    net.addController("c0")

    info("*** Creating hosts\n")
    h1 = net.addDockerHost(
        "h1", dimage="lat_bm", ip="10.0.0.1", docker_args={"hostname": "h1"},
    )
    h2 = net.addDockerHost(
        "h2", dimage="dev_test", ip="10.0.0.2", docker_args={"hostname": "h2"}
    )

    info("*** Adding switch and links\n")
    switch1 = net.addSwitch("s1")
    switch2 = net.addSwitch("s2")
    net.addLink(switch1, h1, bw=10, delay="10ms")
    net.addLink(switch1, switch2, bw=10, delay="10ms")
    net.addLink(switch2, h2, bw=10, delay="10ms")

    info("\n*** Starting network\n")
    net.start()
    print("- Run builder to build FFPP programs.")
    builder = mgr.addContainer(
        "builder",
        "h2",
        "ffpp",
        "bash",
        docker_args={"volumes": FFPP_VOLS, "working_dir": "/ffpp_app",},
    )
    time.sleep(1)
    spawnXtermDocker("builder")
    CLI(net)

    print("- Add the distributor VNF on host h2.")
    distributor = mgr.addContainer(
        "distributor",
        "h2",
        "ffpp",
        "bash",
        docker_args={
            "volumes": FFPP_VOLS,
            "working_dir": "/ffpp_app",
            "nano_cpus": int(3e8),
        },
    )
    time.sleep(1)
    spawnXtermDocker("distributor")
    CLI(net)

    print("- Add the dummy VNF on the host h2.")
    dummy_vnf = mgr.addContainer(
        "dummy_vnf",
        "h2",
        "ffpp",
        "bash",
        docker_args={
            "volumes": FFPP_VOLS,
            "working_dir": "/ffpp_app",
            "nano_cpus": int(3e8),
        },
    )
    spawnXtermDocker("dummy_vnf")
    time.sleep(1)
    CLI(net)

    mgr.removeContainer("distributor")
    mgr.removeContainer("dummy_vnf")
    net.stop()
    mgr.stop()
