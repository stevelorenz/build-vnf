# Fast Packet IO library in User Space (fastio_user) #

This library is a light weight DPDK C wrapper that can be used to speed up packet I/O and processing with more
programmer friendly APIs and utils. See ./examples/ for some sample apps built on top of fastio_user.

## Compiling ##

In order to simplify and the build process on different platforms. The Docker container is suggested to build the
library (To avoid "I could not build this on my laptop due to dependency errors etc"). There is a Dockerfile and a bash
script(./run_dev_container.sh) in the project's root directory to automate the build process. Make sure you have Docker
CE installed. The container image and library can be built with following steps. (Because the DPDK is compiled from the
source code, the step of build the container image takes time, -j can be modified in the Dockerfile to speed up the
compiling time.)

    $ ./run_dev_container.sh -b
    $ ./run_dev_container.sh -m

By default, the compiled library is placed in ./build/ directory.

## Dependencies ##

The dependencies are planned to managed by the Dockerfile. The versions are marked here clearly to avoid confusion.

- DPDK: v18.11 LTS

## Reference Frameworks ##

Some tricks and low level optimizations are learned from several high-performance packet I/O and processing frameworks
and integrated into fastio_user. These frameworks are awesome, but are relative too heavy, hide many tunable
parameters or have some limitations to simply integrated with data processing frameworks (e.g. Using OpenCV's python
bindings to process images easily). Therefore, I have built this light library.

- [VPP](https://wiki.fd.io/view/VPP/What_is_VPP%3F):

    1. Vectorized processing to avoid L1 cache misses and use a
    efficient prefetch mechanism.
