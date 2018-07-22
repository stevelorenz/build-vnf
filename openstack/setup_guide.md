# Setup Guide for the OpenStack Testbed #

## Setup Controller Node ##


## Setup Compute Node ##

1. Connect to pxeboot network and enable network boot.

2. Install Ubuntu server (16.04) with simple-boot.

3. Install and configure OpenStack with devstack.

4. Edit nova.conf and nova-cpu.conf to enable CPU features (>SSE3) required by DPDK.

  - section libvirt:  cpu_mode=host-passthrough

5. Hope nothing goes wrong...
