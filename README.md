# Build VNF #

Try different packet processing tools to build simple Virtualized Network Function (VNFs) in virtual machines or
containers. The main concern of performance is the **low latency**.

Such VNFs should be latter integrated into the [SFC-Ostack](https://github.com/stevelorenz/sfc-ostack) framework.

## Packet Processing Tools ##

1. Data Plane Development Kit (DPDK)
1. IO Visor Project: eBPF + XDP
1. Click Modular Router
1. Linux Packet Socket (AF_PACKET)
1. User-space NIC Driver (TBD)

## TODO: Comparison ##

1. DPDK:

1. XDP:

  - Pros:

  - Cons:

    - Current XDP implementation (Kernel 4.8) can only forward packets back out the same NIC they arrived on.


## Catalog ##

## Evaluation Measurements ##

**Measurements results (in CSV format) are not stored in the Repo, figures can be found ./evaluation/figurs**

## References ##

- [Cilium: BPF and XDP Reference Guide](http://docs.cilium.io/en/latest/bpf/#)
- [Prototype Kernel: XDP](https://prototype-kernel.readthedocs.io/en/latest/networking/XDP/index.html)
