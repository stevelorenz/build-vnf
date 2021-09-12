# FFPP - Fast Functional Packet Processing

It's packet processing library designed for exploring [Computing in the Network (COIN)](https://datatracker.ietf.org/rg/coinrg/about/) use cases.

The project is currently under **heavy revision**. More docs will be added later when the source code is stable.

Common facts about FFPP:

- Cloud-native Network Function (CNF), data plane.

Currently, many emulation/test related code is directly included here.
The plan is to remove them and extend the [ComNetsEmu](https://github.com/stevelorenz/comnetsemu) project to support required functionalities.

One main purpose of this library is to explore and benchmark the latest good features of the used network IO libraries (e.g. DPDK and XDP-Tools).
Therefore, the latest versions are used and some of these libraries have to be **built from source** due to lacking of the deb packages.
This library is **NOT** designed to be used in production with dependencies of stable versions.

## Get Started (WIP)

## Development (WIP)

## Project Structure (WIP)

- benchmark: Programs to benchmark some functions implemented in FFPP.

- Dockerfile: The default Dockerfile to build the development Docker image.

- emulation: Emulation scripts using ComNetsEmu.

- kern: eBPF/XDP programs running in Linux kernel space. Related sources should be mostly GPL licensed.

- Makefile: Makefile for some common tasks including e.g. building the project and run code checkers.

- utils: Utility scripts and tools for internal development purposes.
