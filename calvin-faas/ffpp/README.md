# FFPP - Fast Functional Packet Processing

TODO: Add a fantastic introduction of this (maybe) novel research idea.

## Build from source

**WARNING**: This library also supports GNU/Linux platform and has been tested on Ubuntu 20.02 LTS.

Since FFPP utilizes latest fast packet IO technologies in both Linux kernel and user space, several dependencies must be
installed in the development environment.

Sources running in user space (located in `./user`) are built with Meson.
Sources running in kernel space (located in `./kern`) are built with the makefile-based system provided by the
[xdp-tools](https://github.com/xdp-project/xdp-tools) project.

The following packages and libraries are needed:


*   `Meson (>=0.47.1)`: The standard automatic build system for DPDK, therefore it is also used in this project.
*   `Ninja (>=1.10.0)`: Build system (focus on speed) used by Meson.

*   `DPDK (v20.02)`: [DPDK](https://core.dpdk.org/download/) library should be installed and can be found by
    `pkg-config`. DPDK's API and ABI could change in every release, so this library only supports a specific version.
    Please check DPDK's [guide](https://doc.dpdk.org/guides-20.02/linux_gsg/index.html) to build DPDK for Linux.

*   `xdp-tools (v0.0.3)`: [xdp-tools](https://github.com/xdp-project/xdp-tools) contains a collection of utilities and
    example code to be used with the eXpress Data Path facility of the Linux kernel. It also contains a git submodule
    with [libbpf](https://github.com/libbpf/libbpf). Libbpf is required (be accssible via `pkg-config`) to build the
    AF_XDP PMD of DPDK.

If all dependencies are installed correctly, then you should be able to simply run following commands to build and
install (**ONLY** user space library) both shared and static library of FFPP:

```bash
make
make install
```

## Run tests

Additional required packages to run unit tests:

*   `[Cmocka]`(https://cmocka.org/): An elegant unit testing framework for C and it's written in pure C.
*   `[gcovr]`(https://gcovr.com/en/stable/): To generate coverage report.

Assume the `build` directory is already created using the default options using `make`. Run following commands to clean
the release build, build the debug version and run tests.

```bash
make clean
make debug
make test
```
HTML coverage report can be found at `./user/build/meson-logs/coveragereport/index.html`

## Build and develop in a all-in-one Docker container.

If you do n’t want to waste time setting up the environment, or just want to try it,
a all-in-one [Dockerfile](./Dockerfile) is provided by this project. This image includes DPDK, libbpf and all
dependencies required for development and also testing. Therefore, it has a large size of 1.51GB.
It is only designed for development. For deployment, tricks like multi-stage build can be used to reduce the image size.

Advantages of building all dependencies inside the Container:

*   Keep your host OS clean. At least, I do not know how to remove all dependencies in a nice way.
*   Reproducible build: If there are any dependency updates or some bad conditions occur. Remove the old image and
    rebuild a new one, everything is fine.
*   Build and test VNFs inside container: It is required to run DPDK applications inside container any way.

Run following command to build the Docker image (with tag 0.0.2):

```bash
docker build --compress --rm -t ffpp:0.0.2 --file ./Dockerfile .
```

There are some utility scripts in `./util/` directory for development and tests. Please install dependencies for these
tools with:

```bash
cd ./util/
sudo bash ./setup_devtools.sh
```

After the installation of dependencies, please run following commands **INSIDE** the `./util/` directory for specific
actions:

*   `sudo ./ffpp-dev.py run`: Run a Docker container (with ffpp-dev image) in interactive mode and attach to its TTY. The
    /ffpp path inside container is mounted by the ffpp source directory of the host OS. This is useful to test any
    changes in the code base without any copy. This script also configures Docker parameters (e.g. privilege and
    volumes) to run DPDK and XDP programs. So you do not need to remember and configure them everytime.

*   `sudo ./benchmark-two-direct.py setup` or `sudo ./benchmark-two-direct.py teardown`: Setup/teardown a minimal setup
    to benchmark the performance of a VNF (inside a container) with a pktgen container. Two containers are directly
    conntected using a veth pair.

## Catalog

1.  user: Sources for programs running in the user space.

1.  kern: Sources for eBPF and XDP programs running in the kernel space.

## Development Guide

TODO

## Authors

*   Zuo Xiang (zuo.xiang@tu-dresden.de)

## Licence

The user space code of this project is licensed under the MIT - see the [LICENSE](../../LICENSE) file for details.
The kernel space code of this project is licensed under GPL-2.0.
