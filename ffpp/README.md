# FFPP - Fast Functional Packet Processing

It's a packet processing library (`libffpp`) designed for exploring the research topic: [Computing in the Network (COIN)](https://datatracker.ietf.org/rg/coinrg/about/).
Currently, it only focuses on the **data plane** for packet processing.

The project is currently under **heavy development and revision**.
More docs will be added later when the source code is stable.

## TODO: Build and Run

## Catalog

- Dockerfile, Dockerfile.dev: The Dockerfiles to build release and development Docker images
- Makefile: Makefile to build the project (executables and eBPF object files)
- emulation: Emulation scripts using ComNetsEmu
- examples: Basic examples using libffpp
- include, src: C/C++ source files for libffpp
- kern: eBPF/XDP programs running in Linux kernel space (kern)
- related_works: Programs to reproduce results of related papers (for comparison)
- tests: Test programs for libffpp
- utils: Utility scripts and tools for internal development purposes
