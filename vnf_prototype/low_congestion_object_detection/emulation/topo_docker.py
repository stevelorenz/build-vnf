#!/usr/bin/python2

"""
About: Containernet topology to test YOLO pre-processing VNF
"""

from mininet.net import Containernet
from mininet.node import Controller
from mininet.cli import CLI
from mininet.link import TCLink
from mininet.log import info, setLogLevel

setLogLevel('info')

DPDK_VOLUMES = []
# Required for runnning DPDK inside container
for v in ["/sys/bus/pci/drivers", "/sys/kernel/mm/hugepages",
          "/sys/devices/system/node", "/dev"]:
    DPDK_VOLUMES.append("%s:%s:rw" % (v, v))

net = Containernet(controller=Controller)

info('*** Adding docker containers\n')
d1 = net.addDocker('d1', ip='10.0.0.251', dimage="ubuntu:trusty")
d2 = net.addDocker('d2', ip='10.0.0.252', dimage="darknet",
                   volumes=DPDK_VOLUMES)

info('*** Adding switches\n')
s1 = net.addSwitch('s1')
s2 = net.addSwitch('s2')

info('*** Creating links\n')
net.addLink(d1, s1)
net.addLink(s1, s2, cls=TCLink, delay='100ms', bw=1)
net.addLink(s2, d2)

info('*** Starting network\n')
net.start()
info('*** Testing connectivity\n')
net.ping([d1], manualdestip="10.0.0.252")
info('*** Running CLI\n')
CLI(net)
info('*** Stopping network')
net.stop()
