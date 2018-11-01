# Build VNF #

## Motivation ##

Test and evaluate different packet IO and processing tools for implementing Virtualized Network Function (VNFs) on
virtual machines, Unikernels or containers. The main focus of performance here is the **low latency** (End-to-End
service delay). Pros and cons of each tool should be studied and latter utilized for building VNF with different
requirements.

For example, firewalls or load balancers can be built using tools that provide high IO speeds but limited computing
power.  For compute-intensive network functions, such as data encryption, network coding or compressed sensing. The
processing ability and delay performance of the tool should also be considered.

Such VNFs can be latter e.g. integrated into NFV platforms or frameworks.

Check out the [draft of our paper](./paper/paper_draft.pdf) to get more detailed information.

## Catalog ##

Since this repo is research oriented, many PoCs or prototypes are also included. These sub-items are classified in the
[tech](./techs/) directory according to the technology used. For instance, the [io\_visor](./techs/io_visor/) directory
contains prototypes and helper scripts to develop VNFs with tools provided by IO Visor project. These codes are not
guaranteed to be stable and also upgraded with latest features. Relative mature and stable VNFs are put into the
[vnf\_release](./vnf\_release/) directory with source code and also usage guide. These sub-items will be maintained and
improved with latest features. The unstable VNFs under development are located in [vnf\_debug](./vnf\_debug/) directory.
Detailed catalog information SHOULD be found in the documentation.

## Getting Started with Mininet and an Example ##

Currently, there is a simple example of building a Service Function Chain (SFC) with the famous
[Mininet](http://mininet.org/) emulator and processing packets with raw socket and AF_PACKET in Python.
Check the files [here](./vnf_debug/pedestrian_detection/emulation/) for details.

## Documentation ##

[Sphinx](http://www.sphinx-doc.org/en/master/) is used to build the documentation. Install Sphinx before building the
documentation source files in ./doc/.

For Linux user, run following commands to build the HTML documentation:

```bash
cd ./doc/
make html
xdg-open ./doc/build/html/index.html
```

## Packet IO Frameworks ##

1. Data Plane Development Kit (DPDK)

1. IO Visor Project --- BCC: eBPF + XDP

1. Click Modular Router (Only use the user-space driver)

1. Linux Packet Socket (AF_PACKET)

1. Linux XDP Socket (AF_XDP) (TBD, New since Linux v4.18)
<!--1. User-space NIC Driver: [IXY](https://github.com/emmericp/ixy) (TBD)-->

## Packet Processing Frameworks ##

1. [Kodo](http://steinwurf.com/products/kodo.html): Kodo makes it easy to deploy Erasure Correcting Codes in your
application.

1. Network Coding Kernel Library (NCKernel): This library contains encoders and decoders that can be used for network
coded communications.

1. [kokke/tiny-AES-c](https://github.com/kokke/tiny-AES-c): Portable AES implementation in C

## Evaluation Measurements ##

- **Measurements results (in CSV format) are not stored in the Repo, figures can be found ./evaluation/figurs**

- Measurements tools can be found in ./delay_timer/ , ./perf/

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

## Main References ##

- [Cilium: BPF and XDP Reference Guide](http://docs.cilium.io/en/latest/bpf/#)
- [Prototype Kernel: XDP](https://prototype-kernel.readthedocs.io/en/latest/networking/XDP/index.html)
- [DPDK Developer Documentation](http://doc.dpdk.org/guides/prog_guide/)
- [BCC Reference Guide](https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md)
- [Scaling virtual machine network performance for network intensive workloads](https://www.redhat.com/blog/verticalindustries/scaling-virtual-machine-network-performance-for-network-intensive-workloads/)
