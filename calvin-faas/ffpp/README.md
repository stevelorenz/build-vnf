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

## Run a Docker container, DPDK and XDP based benchmark topology on a single laptop

### Requirements

-   The test VM or host machine needs minimal 2 vCPU cores.
-   The `ffpp-dev` container image is already built.
-   The test VM or host machine requires some setup. You can run `./util/setup_host_os.sh` script to setup all
    dependencies if the Vagrant VM is used.

### Topology

The benchmark creates two Docker containers on which a packet generator and a network function will run.
Like the following sketch, each container has two veth interfaces.
Their corresponded veth pair interfaces have the same name with `-root` suffix.
The IP addresses of each interface are listed.
The packet generator should generate traffic on `pktgen-out` interface.
Then the traffic can be forwarded by DPDK l2fwd sample application with `AF_XDP` PMD on the interface `vnf-in-out`.
Then the forwarded traffic is redirected by `vnf-in-out-root` interface back to the `pktgen-in` interface inside pktgen.
This setup build a minimal Traffic Generator --- DUT (Device under Test) scenario.

```
Pktgen Container                            VNF Container (vnf-out is not used currently)
-----------------------------------         -------------------------------------
|    pktgen-out (IP: 192.168.17.1) |        |    vnf-in-out (IP: 192.168.17.2)   |
|    pktgen-in  (IP: 192.168.18.1) |        |    vnf-out (IP: 192.168.18.2)      |
---------------||------------------         ----------------||--------------------

        pktgen-out-root --------------------------------->  vnf-in-out-root
        pktgen-in-root  <---------------------------------  vnf-in-out-root
                                  xdp_fwd program
```

### How to run

Please change the current working directory to `./util/` before running following commands.

1.  Setup the benchmark:

```bash
sudo ./benchmark-two-direct.py --setup_name two_veth_xdp_fwd setup
```

2.  Attach to pktgen and vnf container and start programs. This step needs multiple terminals, Tmux or Screen can be
    used:

```bash
# Run DPDK l2fwd with AF_XDP PMD on VNF container
terminal 1 > sudo docker attach vnf
terminal 1 > cd ./util/ && bash ./run_dpdk_l2fwd_af_xdp.sh

# Generate simple ICMP traffic on pktgen-out interface.
terminal 2 > sudo docker attach pktgen
terminal 2 > ping 192.168.17.2

# Run the dummy xdp_pass program on pktgen-in interface and dump received traffic.
terminal 3 > sudo docker exec -it pktgen bash
terminal 3 > cd ./kern/xdp_pass/ && make && xdp-loader load -m native pktgen-in ./xdp_pass_kern.o
terminal 3 > sudo tcpdump -e -i pktgen-in
```

If everything is configured correctly, you should see ICMP packets generated from `pktgen-out` interface are dumped by
tcpdump on `pktgen-in` interface.

3.  Tear down the setup with cleanup.

```bash
sudo ./benchmark-two-direct.py --setup_name two_veth_xdp_fwd teardown
```


## Catalog

1.  user: Sources for programs running in the user space.

1.  kern: Sources for eBPF and XDP programs running in the kernel space.

## Development Guide

TODO

## FAQ

TODO

## Authors

*   Zuo Xiang (zuo.xiang@tu-dresden.de)

## Licence

The user space code of this project is licensed under the MIT - see the [LICENSE](../../LICENSE) file for details.
The kernel space code of this project is licensed under GPL-2.0.
