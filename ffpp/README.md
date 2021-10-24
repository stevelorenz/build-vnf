# FFPP - Fancy Fast Packet Processing

FFPP is a Packet Processing (PP) library developed to explore the use cases of the research topic COIN.
The project is currently under **heavy development and revision**.
More docs will be added later when the structure and source code are stable.

I'm currently exploring rewriting FFPP in Go or Rust.

## TODO: Build and Run

## Catalog

- Dockerfile: The Dockerfile to build the Docker image for FFPP
- Makefile: Makefile to build the project
- emulation: Emulation scripts using [ComNetsEmu](https://github.com/stevelorenz/comnetsemu)
- examples: Examples using libffpp (under **revision**)
- include, src: C/C++ source files for libffpp
- kern: eBPF/XDP programs running in Linux kernel (kern) space
- related\_works: Programs to reproduce results of related papers (for comparison)
- tests: Test programs for libffpp
- scripts: Utility scripts and tools
