# FFPP - Fancy Fast Packet Processing

FFPP is a Packet Processing (PP) library developed primarily to (at least, try to) explore the research topic [Computing in the Network](https://datatracker.ietf.org/rg/coinrg/about/).
Another research target is to exploit the judicious synergistic collaboration of kernel space eBPF/XDP and user space DPDK for NFV software dataplane network.

It was also an opportunity (a challenge) for me to re-implement the wheel and learn how pure software packet processing system works.
Why re-implement the wheel ???
🤓 Check [build-your-own-x](https://github.com/danistefanovic/build-your-own-x).

The structure of the source tree is similar to that of the [DPDK](https://git.dpdk.org/dpdk/tree/) project.
FFPP is designed and implemented to build Containerized (or Cloud-native) Network Functions (CNF).
So it should be mainly used inside a container.
The project provides a sample [Dockerfile](./Dockerfile) to build a base image to prototype network functions on top of FFPP.
The sample image is only used for prototyping, so layers and image size are not optimized at all.

The project is currently still under **heavy development and revision**.
More documentation will be added later when the structure and source tree are stable.

## Build FFPP Docker Image (Recommended)

The Docker image contains all dependencies required to build and install all FFPP targets.

Use the script to build FFPP using BuildKit of Docker:
```bash
bash ./build_image.sh
```

## Build FFPP Targets from Source

FFPP can be configured, built and installed using the tools [Meson](https://mesonbuild.com/) and Ninja.
Check the Dockerfile or meson.build for required dependencies.
All dependencies should be installed before building from source.

Use the default release profile to build and install FFPP system-wide:

```bash
meson build --buildtype=release
cd ./build
ninja
ninja install
ldconfig
```

## TODO: Add more information in the README.
