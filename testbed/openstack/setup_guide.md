# (Archive) Setup Guide for the OpenStack Testbed #

1.  Use OVS-DPDK for accelerating Neutron bridges.

2.  Use DPDK to implement VNFs in the VM.


## Setup Controller Node ##


## Setup Compute Node ##

1.  Connect to pxeboot network and enable network boot.

2.  Install Ubuntu server (16.04) with simple-boot.

3.  Install and configure OpenStack with devstack.

4.  Edit nova.conf and nova-cpu.conf to enable CPU features (>SSE3) required by DPDK:
    section libvirt: cpumode=host-passthrough

5.  Hope nothing goes wrong...


## Setup Images and Flavors ##

In order to connect VMs to vhost-user interfaces (used by OVS-DPDK fast path), the flavor should be configured with the
property *hw:mem_page_size=large*. The property *hw:vif_multiqueue_enabled='true'* should be configured for both flavor
and images to enable multiple queue support of vhost-user interfaces. Multiple queue is also required to run XDP
programs.

## Setup Evaluation Experiments ##

-   Static ARPs should be configured on each VM when its interface is bound to DPDK (DPDK has no built-in stack, ARP
    requests can not be handled).
