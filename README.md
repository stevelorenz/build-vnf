# Build VNF #

## Build Low Latency Virtualized Network Functions ##

This repository is under heavy development.
The directories are not cleanly and clearly organized.
Redundant directories will be removed before the stable 1.0 version.

This project is hosted on [GitHub](https://github.com/stevelorenz/build-vnf) and [BitBucket](https://bitbucket.org/comnets/build-vnf/src/master/). 
Please create an issue or a pull request if you have any questions or code contribution.

## Introduction ##

This repository is a **mono-repo collection** of programs/libraries/scripts/utilities developed and evaluated for
[my PhD](https://cn.ifn.et.tu-dresden.de/chair/staff/zuo-xiang/) topic: Enabling Ultra Low Latency Network Softwarization.

These programs are used to prototype and benchmark innovation ideas and

These programs are used to prototype and evaluate innovation ideas and practical approaches for ultra low latency data
and management plane of Virtualized Network Function (VNFs) running inside virtual machines (VMs) and containers (CNCF
names this type of NFs to Cloud-native Network Functions (CNFs)) that are deployed on Commercial off-the-shelf (COTS) servers.

The ultra low latency is a fundamental requirement for the [Tactile Internet](https://www.telekom.com/en/company/details/tactile-internet-563646).
According to the 5G communication standard for automation in vertical domains, the allowed end-to-end communication
latency should be lower than **1ms**.
Based on the latency budget described in [this paper](https://ieeexplore.ieee.org/abstract/document/8672612), the
virtualized edge computing system has only **0.35ms** for communication and computation.
Furthermore, one killer application for Tactile Internet is to enable Human-machine Co-Working.
In this use-case, the service delay of **each discrete packet** play an important role.
According to the [networking requirements of Franka Emika's Robot Arm](https://frankaemika.github.io/docs/requirements.html#network),
the latency of the full control loop should less than 1ms.

> If the <1 ms constraint is violated for a cycle, the received packet is dropped by FCI. After 20 consecutively dropped packets, your robot will stop.

Therefore, the latency of each discrete packet should be measured as the KPI for this project.
Besides latency, other important performance metrics including throughput, power efficiency should also be considered in
the VNF design and implementation.

This is a PhD student's work, so the code is definitely not production-ready.
However, I will try my best to adapt my work to the latest high performance softwarization dataplane and management
technologies including DPDK, eBPF/XDP; Containerization, Serverless computation etc.

## Catalog ##

1.  [app](./app/): Prototypes for innovative applications/functions that can be offloaded to the network.
    They are inspired by the [Computing-In-Network](https://irtf.org/coinrg) concept.
1.  [benchmark](./benchmark/): Performance benchmarks' setups and utility scripts.
1.  [CALVIN](./CALVIN/): Programs and scripts used in [JSAC CALVIN paper](https://ieeexplore.ieee.org/abstract/document/8672612).
1.  [evaluation](./evaluation/): Scripts to analyze and plot evaluation results.
1.  [ffpp](./ffpp): A simple research-oriented library to build Fast Fast Packet Processing (FFPP) VNFs.
1.  [nff](./nff/): Tests and examples built on top of popular Network Function (NF) frameworks.
1.  [perf](./perf/): A collection of tools to monitor and profiling the underlying physical or virtual systems running VNFs.
1.  [pktgen](./pktgen/): Scripts to configure and run software packet generators for latency measurement.
1.  [scripts](./scripts/): Utility shell/python scripts to install, setup, configure tools used in this project.
1.  [vnf_prototype](./vnf_prototype/): VNFs under prototyping (Try new ideas).
    **Not functionally correct and stable** and the performance is not fine tuned.
1.  [vnf_release](./vnf_release/): Tested and performance-optimized VNFs.


### To be cleaned/removed Directories ###

1.  [shared_lib](./shared_lib/): Initially used to put shared libraries used by different VNFs. Should be integrated
    into fpp library.
1.  [techs_test](./techs_test/): Initially used to test different high-performance IO technologies when I started my PhD
    into this topic. The technologies chosen to be further researched is eBPF/XDP and DPDK.

## Documentation and Development Guide ##

This repository contains a development documentation whose source files (use reStructuredText format) are located in
[./doc/source/](./doc/source/). [Sphinx](http://www.sphinx-doc.org/en/master/) is used to build the documentation.
Install requirements via `pip install -r ./doc/requirements.txt` before building the documentation source files.
On a Unix-like system, the documentation (HTML format) can be built and opened with:

```bash
$ make doc-html
$ xdg-open ./doc/build/html/index.html
```

## Contribution ##

This project exists thanks to all people who contribute.
The [list](./CONTRIBUTORS) of all contributors.

## Contact ##

* **Zuo Xiang** - *Project Maintainer* - [Email him]((mailto:xianglinks@gmail.com?subject=[GitHub]%20Build%20VNF%20Issue))
