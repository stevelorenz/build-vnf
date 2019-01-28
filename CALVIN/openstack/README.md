# Setup Guide for the OpenStack Testbed #

The [devstack](https://docs.openstack.org/devstack/latest/) is used to setup the OpenStack environment for CALVIN.

Currently setup requires following plugins (extensions):

1. [Service Function Chaining Extension for OpenStack Networking (networking-sfc)](https://docs.openstack.org/networking-sfc/latest/)

2. [OpenStack Open vSwitch with DPDK datapath (OVS-DPDK)](https://docs.openstack.org/ocata/networking-guide/config-ovs-dpdk.html)

The installation of these plugins (extensions) is included in the ./controller.conf and ./compute.conf files.

The orchestration of SFCs on OpenStack is currently managed by the light weight and research-oriented framework
[sfc-ostack](https://github.com/stevelorenz/sfc-ostack). For advanced and general usages, the OpenStack official OpenStack NFV MANO
framework [tacker](https://docs.openstack.org/tacker/latest/) is recommended.

## Setup Controller Node ##

1. Connect to PXE-Boot network and enable network boot.

2. Install Ubuntu server (16.04) with PXE-Boot service.

3. Install and configure OpenStack with devstack.

4. Download images for spawning new VMs, e.g. downloading [ubuntu cloud images](https://cloud-images.ubuntu.com/)

## Setup Compute Node(s) ##

1. Connect to PXE-Boot network and enable network boot.

2. Install Ubuntu server (16.04) with PXE-Boot service.

3. Install and configure OpenStack with devstack.

4. Edit nova.conf and nova-cpu.conf to enable CPU features (>SSE3) required by DPDK:
section libvirt: cpumode=host-passthrough

5. Hope nothing goes wrong...

## Setup Images and Flavors ##

In order to connect VMs to vhost-user interfaces (used by OVS-DPDK fast path), the flavor should be configured with the
property *hw:mem_page_size=large*. The property *hw:vif_multiqueue_enabled='true'* should be configured for both flavor
and images to enable multiple queue support of vhost-user interfaces. Multiple queue is also required to run XDP
programs.

## Setup Evaluation Experiments ##

- Static ARPs should be configured on each VM when its interface is bound to DPDK (DPDK has no built-in layer 3
    networking stack, ARP requests can not be handled by default).
