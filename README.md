# Build VNF #

This repository is under heavy development. The directories are not cleanly and clearly organized.
Redundant directories will be removed before the stable 1.0 version. The README will also be updated.

This project is hosted on [GitHub](https://github.com/stevelorenz/build-vnf) and
[BitBucket](https://bitbucket.org/comnets/build-vnf/src/master/), please create an issue or a pull request if you have
any questions or code contribution.

## Introduction ##

This repository is a **collection** of programs/libraries/script tools developed and evaluated for [my
PhD](https://cn.ifn.et.tu-dresden.de/chair/staff/zuo-xiang/) topic: Enabling Ultra Low Latency Network Softwarization.

These programs are used to prototype and evaluate innovation ideas and practical approaches for low latency data and
management plane of Virtualized Network Function (VNFs) running inside virtual machines and containers on a COTS server
or on programmable data plane switches.
The main focus of performance here is the **low latency** (End-to-End service delay) and energy efficiency.

## Catalog ##

1. [CALVIN](./CALVIN/): Programs and scripts used in [JSAC CALVIN paper](https://ieeexplore.ieee.org/abstract/document/8672612).
1. [benchmark](./benchmark/): Performance benchmarks' setups and utility scripts.
1. [dockerfiles](./dockerfiles/): A collection of Dockerfiles for **base images** used by building and benchmarking
VNFs.  This includes common-used (e.g. DPDK) and also my own (e.g. fastio user) libraries, traffic generators,
benchmark/perf frameworks etc. Some VNF Dockerfiles in prototype and release folder use these base images.
1. [evaluation](./evaluation/): Scripts to plot evaluation results.
1. [fastio_user](./fastio_user/): A low-level C library for packet IO and processing in user space.
1. [nff](./nff/): Tests and examples built on top of popular NF/VNF frameworks.
1. [scripts](./scripts/): Utility shell scripts to install, setup, configure tools used in this project.
1. [vnf_prototype](./vnf_prototype/): VNFs under prototyping (Try new ideas). **Not stable** and the performance is not fine tuned.
1. [vnf_release](./vnf_release/): Tested and performance-optimized VNFs.
1. [pktgen](./pktgen/): Scripts to configure and run software packet generators for latency measurement.

## Documentation ##

@TODO: Update documentation and remove unused sections.

This repository contains a tutorial/Wiki like documentation whose source files (use reStructuredText format) are located
in [./doc/source/](./doc/source/). [Sphinx](http://www.sphinx-doc.org/en/master/) is used to build the documentation.
Install Sphinx before building the documentation source files. On a Unix-like system, the documentation (HTML format)
can be built and opened with:

```bash
$ make doc-html
$ xdg-open ./doc/build/html/index.html
```

## Contributors ##

1. Zuo Xiang: Maintainer.
2. Funing Xia: Add Lua scripts for latency measurement with MoonGen.
3. Renbing Zhang: Add YOLO-based object detection pre-processing APP.
