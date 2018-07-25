# Build VNF #

Test and evaluate different packet IO and processing tools for implementing Virtualized Network Function (VNFs) on
virtual machines, Unikernels or containers. The main focus of performance here is the **low latency** (End-to-End
service delay). Pros and cons of each tool should be studied and latter utilized for building VNF with different
requirements.
For example, firewalls or load balancers should be built using tools that provide high IO speeds but limited computing
power.
For compute-intensive network functions, such as data encryption, network coding and compressed sensing. The processing
ability and delay performance of the tool should also be considered.

Such VNFs should be latter integrated into the [SFC-Ostack](https://github.com/stevelorenz/sfc-ostack) framework.

## Packet IO Frameworks ##

1. Data Plane Development Kit (DPDK)
1. IO Visor Project: eBPF + XDP
1. Click Modular Router (Use user-space driver)
1. Linux Packet Socket (AF_PACKET)
1. Linux XDP Socket (AF_XDP) (TBD, New since Linux v4.18)
1. User-space NIC Driver: [IXY](https://github.com/emmericp/ixy) (TBD)

## Packet Processing Frameworks ##

1. Network Coding Kernel Library (NCKernel): Used for network coded communications.

## TODO: Catalog ##

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
          of vCPUs in the guest (use virtio-net driver). Since XDP requires an additional TX queue per core which is not
          visible to the networking stack.  Therefore, currently XDP can not be deployed in VMs on OpenStack directly.


3. AF_PACKET:

  - Pros:

      - Built-in feature of Linux Kernel. No additional configuration or hardware features are required.

  - Cons:

      - Current AF_PACKET(V3) does not support zero-copy, lock-less structures, eliminations of syscalls for high IO
          speed.


4. Click Modular Router:


## Evaluation Measurements ##

**Measurements results (in CSV format) are not stored in the Repo, figures can be found ./evaluation/figurs**

## Version of used Tools ##

- Linux Distribution: Vagrant box: bento/ubuntu-16.04

- Linux Kernel:

    - For BCC: 4.17.0-041700-generic
    - For DPDK, Click: Default version in bento/ubuntu-16.04

- DPDK: v18.02-rc4
- BCC: v0.6.0
- Click: v2.0.1

- OpenStack: Pike with OVS-DPDK

    - Nova: https://github.com/stevelorenz/nova/tree/pike-dev-zuo

- [kokke/tiny-AES-c](https://github.com/kokke/tiny-AES-c): commit 3410accdc35e437ea03e39f47d53909cbc382a8e

- NCKernel: commit: bf973e0 (2018-06-30)

## References ##

- [Cilium: BPF and XDP Reference Guide](http://docs.cilium.io/en/latest/bpf/#)
- [Prototype Kernel: XDP](https://prototype-kernel.readthedocs.io/en/latest/networking/XDP/index.html)
- [DPDK Developer Documentation](http://doc.dpdk.org/guides/prog_guide/)
- [BCC Reference Guide](https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md)
- [Scaling virtual machine network performance for network intensive workloads](https://www.redhat.com/blog/verticalindustries/scaling-virtual-machine-network-performance-for-network-intensive-workloads/)
