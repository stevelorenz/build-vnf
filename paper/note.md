# Note #

## OVS-DPDK ##

- Significantly faster (about 3 times compared to kernel path) packet processing.
    Especially additional packet modifications are performed instead of simple forwarding.

- Current version of OVS-DPDK does not support HW offloading. Therefore, VMs (connected via vhost-user interface)
    Calculate CKSUMs and process ethernet frames one by one.
