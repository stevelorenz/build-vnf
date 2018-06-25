# Build VNF #

Try different packet processing tools to build simple Virtualized Network Function (VNFs) in virtual machines or
containers. The main concern of performance is the **low latency**.

Such VNFs should be latter integrated into the [SFC-Ostack](https://github.com/stevelorenz/sfc-ostack) framework.

## Packet Processing Tools ##

1. Data Plane Development Kit (DPDK)
1. IO Visor Project: eBPF + XDP
1. Click Modular Router
1. Linux Packet Socket (AF_PACKET)
1. Linux XDP Socket (AF_XDP) (TBD)
1. User-space NIC Driver (TBD)

## TODO: Comparison ##

1. DPDK:


1. XDP:

  - Pros:

  - Cons:

    - Current XDP implementation (Kernel 4.8) can only forward packets back out the same NIC they arrived on.

    - For OpenStack Pike, the maximum number of queues in the VM interface has to be set to the same value as the number
        of vCPUs in the guest (use virtio-net driver). Since XDP requires an addtional TX queue per core which is not
        visible to the networking stack.  Therefore, currently XDP can not be deployed in VMs on OpenStack directly.


1. AF_PACKET:

  - Pros:

      - Built-in feature of Linux Kernel. No additional configuration or hardware features are required.

  - Cons:

      - Current AF_PACKET(V3) does not support zero-copy, lock-less structures, eliminations of syscalls for high IO
          speed.


1. Click Modular Router:

## Catalog ##

## Evaluation Measurements ##

**Measurements results (in CSV format) are not stored in the Repo, figures can be found ./evaluation/figurs**

## Version of used Tools ##

- DPDK: v17.11-rc4
- BCC: v0.6.0
- Click: v2.0.1

- OpenStack: Pike with OVS-DPDK

    - Nova: https://github.com/stevelorenz/nova/tree/pike-dev-zuo

- [kokke/tiny-AES-c](https://github.com/kokke/tiny-AES-c): commit 3410accdc35e437ea03e39f47d53909cbc382a8e


## References ##

- [Cilium: BPF and XDP Reference Guide](http://docs.cilium.io/en/latest/bpf/#)
- [Prototype Kernel: XDP](https://prototype-kernel.readthedocs.io/en/latest/networking/XDP/index.html)

## TODO ##

- Use a make based compile system for eBPF and XDP programs. (Currently bcc is used.)
- Use Intel AES New Instructions.
