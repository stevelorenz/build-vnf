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

## Brief Comparison ##

1. DPDK:


2. XDP:

  - Pros:

      - XDP provides bare metal packet processing at the lowest point in the software stack.

      - New functions can be implemented dynamically with the integrated fast path without kernel modification.

      - It does not replace the kernel TCP/IP stack and can utilize all features provided by the Kernel networking
          stack.

  - Cons:

      - Current XDP implementation (Kernel 4.8) can only forward packets back out the same NIC they arrived on.
          (Update) The XDP REDIRECT operation works on Kernel 4.17.

      - Implementation pitfalls and limitations: Check the BPF documentation provided by the Cilium.

      - For OpenStack Pike, the maximum number of queues in the VM interface has to be set to the same value as the number
          of vCPUs in the guest (use virtio-net driver). Since XDP requires an addtional TX queue per core which is not
          visible to the networking stack.  Therefore, currently XDP can not be deployed in VMs on OpenStack directly.


3. AF_PACKET:

  - Pros:

      - Built-in feature of Linux Kernel. No additional configuration or hardware features are required.

  - Cons:

      - Current AF_PACKET(V3) does not support zero-copy, lock-less structures, eliminations of syscalls for high IO
          speed.


4. Click Modular Router:

## Catalog ##

## Evaluation Measurements ##

**Measurements results (in CSV format) are not stored in the Repo, figures can be found ./evaluation/figurs**

## Version of used Tools ##

- Linux Distribution: Vagrant box: bento/ubuntu-16.04

- Linux Kernel:

    - For BCC: 4.17.0-041700-generic
    - For DPDK, Click: Default version in bento/ubuntu-16.04

- DPDK: v17.11-rc4
- BCC: v0.6.0
- Click: v2.0.1

- OpenStack: Pike with OVS-DPDK

    - Nova: https://github.com/stevelorenz/nova/tree/pike-dev-zuo

- [kokke/tiny-AES-c](https://github.com/kokke/tiny-AES-c): commit 3410accdc35e437ea03e39f47d53909cbc382a8e

## References ##

- [Cilium: BPF and XDP Reference Guide](http://docs.cilium.io/en/latest/bpf/#)
- [Prototype Kernel: XDP](https://prototype-kernel.readthedocs.io/en/latest/networking/XDP/index.html)
- [DPDK Developer Documentation](http://doc.dpdk.org/guides/prog_guide/)
- [BCC Reference Guide](https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md)

## TODO List ##

- Use a make based compile system for eBPF and XDP programs. (Currently bcc is used.)
- Use Intel AES New Instructions.
