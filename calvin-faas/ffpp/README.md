# FFPP - Fast Functional Packet Processing

TODO: Add a fantastic introduction of this (maybe) novel research idea.

## Build from source

**WARNING**: This library also supports GNU/Linux platform and has been tested on Ubuntu 20.02 LTS.

Since FFPP utilizes latest fast packet IO technologies in both Linux kernel and user space, several dependencies must be
installed in the development environment.

Sources running in user space (located in `./user`) are built with Meson.
Sources running in kernel space (located in `./kern`) are built with plain makefiles.

The following packages and libraries are needed:


*   `Meson (>=0.47.1)`: The standard automatic build system for DPDK, therefore it is also used in this project.
*   `Ninja (>=1.10.0)`: Build system (focus on speed) used by Meson.

*   `DPDK (v20.02)`: [DPDK](https://core.dpdk.org/download/) library should be installed and can be found by
    `pkg-config`. DPDK's API and ABI could change in every release, so this library only supports a specific version.
    Please check DPDK's [guide](https://doc.dpdk.org/guides-20.02/linux_gsg/index.html) to build DPDK for Linux.

*   `libbpf (v.0.0.8)`: [libbpf](https://github.com/libbpf/libbpf) is for development and loading of XDP programs and
    DPDK's AF_XDP PMD deriver.
    Besides `libbpf`, several dependencies are required to compile and load XDP programs, please check the
    [guide](https://github.com/xdp-project/xdp-tutorial/blob/master/setup_dependencies.org#based-on-libbpf) to setup all
    required dependencies.

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

Run following command to build the Docker image:

```bash
docker build --compress --rm -t ffpp:0.0.1 --file ./Dockerfile .
```

## Catalog

1.  user: Sources for programs running in the user space.

1.  kern: Sources for eBPF and XDP programs running in the kernel space.

## Development Guide

TODO

## Authors

*   Zuo Xiang (zuo.xiang@tu-dresden.de)

## Licence

This project is licensed under the MIT - see the [LICENSE](../../LICENSE) file for details
