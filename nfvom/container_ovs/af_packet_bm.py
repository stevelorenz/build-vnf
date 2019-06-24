#!/usr/bin/python2

"""
About: Testbench with af_packet interfaces
"""

from mininet.net import Containernet
from mininet.node import Controller
from mininet.cli import CLI
from mininet.link import TCLink
from mininet.log import info, setLogLevel

setLogLevel('info')

FASTIO_USER_VOLUMES = []
# Required for runnning DPDK inside container
for v in ["/sys/bus/pci/drivers", "/sys/kernel/mm/hugepages",
          "/sys/devices/system/node", "/dev"]:
    FASTIO_USER_VOLUMES.append("%s:%s:rw" % (v, v))

FASTIO_USER_VOLUMES.append("/vagrant/fastio_user/:/fastio_user:rw")

net = Containernet(controller=Controller)
net.addController("c0")

info('*** Adding docker containers\n')
d1 = net.addDocker('d1', ip='10.0.0.1/24', dimage="ubuntu:trusty",
                   cpuset_cpus="0")
d2 = net.addDocker('d2', ip='10.0.0.2/24', dimage="fastio_user",
                   volumes=FASTIO_USER_VOLUMES,
                   cpuset_cpus="1")

info('*** Adding switches\n')
s1 = net.addSwitch('s1')
s2 = net.addSwitch('s2')

info('*** Creating links\n')
net.addLink(d1, s1)
net.addLink(s1, s2, cls=TCLink, delay='10ms', bw=1)
net.addLink(s2, d2)

info('*** Starting network\n')
net.start()
net.ping([d1, d2])

info('*** Running CLI\n')
CLI(net)

info('*** Stopping network')
net.stop()
