# FFPP - Fast Functional Packet Processing

The project is currently under **heavy revision**. More docs will be added later when the source code is stable.

Currently, many emulation/test related code is directly included here.
The plan is to remove them and extend the ComNetsEmu project to support required functionalities.

One main purpose of this library is to explore and benchmark the latest good features of the used network libraries (e.g. DPDK and XDP-Tools).
Therefore, the latest versions are used and some of these libraries have to be compiled from source due to lacking of the deb packages.
This library is **NOT** designed to be used in production with dependencies of stable versions.
