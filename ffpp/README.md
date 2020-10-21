# FFPP - Fast Functional Packet Processing

## Table of Contents

*   [Overview](#overview)
*   [Catalog](#catalog)
*   [Quick Start](#quick-start)
*   [Run Local Benchmark](#run-local-benchmark)
*   [FAQ](#faq)
*   [Contact](#contact)
*   [License](#licence)


## Overview

TODO: Add a fantastic introduction of this (maybe) novel research idea.


## Catalog

-   [user](./user/): Sources for components running in the user space.

    -   `user/comparison/`: Sources for comparable approaches published by others. They are used to evaluate the performance of the `FFPP`'s solutions.

-   [kern](./kern/): Sources for eBPF and XDP programs running in the kernel space. (Currently, it also contains sources to load/unload XDP programs and XDP user space programs, these codes should be moved to `user` soon.)

-   [emulation](./emulation/): Emulation scripts using [ComNetsEmu](https://git.comnets.net/public-repo/comnetsemu).


## Quick Start

The easiest way to start developing `FFPP` is to use the provided [Dockerfile](./Dockerfile).
This image includes all dependencies required for development and also testing.
Therefore, it has a large size of 2GB.
It is only designed for **development**.
For deployment, tricks like multi-stage build can be used to reduce the image size.

Inside the `Vagrant` VM (better with a clean state), run following commands to install dependencies and build the image:

```bash
cd /vagrant/ffpp/util
sudo ./setup_devtools.sh
# This step takes time to compile DPDK and FFPP libraries.
sudo ./ffpp-dev.py build_image
```


## Run Local Benchmark

This local setup based on veth pairs is used to fast test and benchmark network functions on a single laptop with limited computing resources.
Therefore, containers are used with privileged mode to enable running `DPDK` and `XDP` based network functions **INSIDE** them.
This is definitely not suggested for production deployment.

### Requirements

-   The test VM or host machine needs minimal two vCPU cores.
-   The `ffpp-dev` container image is already built.
-   The [Trex](https://trex-tgn.cisco.com/) traffic generator (pktgen) container image is built with the helper script [here](../../pktgen/trex/build_docker_image.sh).
-   The test VM or host machine requires some setup. You can run `./util/setup_host_os.sh` script to setup all dependencies if the Vagrant VM is used.

In summary, once inside the `Vagrant` VM run following commands for setup:

```bash
cd /vagrant/ffpp/util
sudo ./setup_host_os.sh

cd /vagrant/pktgen/trex
sudo ./build_docker_image.sh

sudo docker image ls
# Both ffpp-dev and trex images should be available now.
```

### Topology

The benchmark creates two Docker containers on which a `pktgen` and a `VNF` will run.
Like the following sketch, each container has two `veth` interfaces.
Their corresponded `veth` pair interfaces have the same name with `-root` suffix.
The IP addresses of each interface are listed.
The packet generator should generate traffic on `pktgen-out` interface.
Then the traffic can be forwarded by `DPDK`'s built-in `l2fwd` sample application between the interfaces `vnf-in` and `vnf-out`.
Then the forwarded traffic is redirected by `vnf-out-root` interface back to the `pktgen-in` interface inside pktgen.
This setup build a minimal Traffic Generator --- `DUT` (Device under Test) scenario.

```
Pktgen Container                            VNF Container
-----------------------------------         -------------------------------------
|    pktgen-out (IP: 192.168.17.1) |        |    vnf-in (IP: 192.168.17.2)       |
|    pktgen-in  (IP: 192.168.18.1) |        |    vnf-out (IP: 192.168.18.2)      |
---------------||------------------         ----------------||--------------------

        pktgen-out-root <--------------------------------->  vnf-in-root

        pktgen-in-root  <--------------------------------->  vnf-out-root
                                  xdp_fwd program
```

### How to run

Please change the current working directory to `./util/` before running following commands.
The `/etc/trex_cfg.yaml` in the Docker image built by the helper script is already configured according to the topology using in this benchmark.

1.  Setup the benchmark:

```bash
sudo ./benchmark-local.py --pktgen_image trex:v2.81 setup
```

2.  Attach to `pktgen` and `vnf` container and run following commands. This step needs multiple terminals, `Tmux` or `GNU Screen` can be used:

```bash
# Start Trex server on pktgen
terminal 1 > sudo docker attach pktgen
terminal 1 > cd /trex/v2.81/
terminal 1 > ./t-rex-64 -i
# Wait until Trex server fully started.

# Run DPDK l2fwd program in vnf
terminal 2 > sudo docker attach vnf
# Use AF_Packet PMD just for test
terminal 2 > ./util/run_dpdk_l2fwd_af_xdp.sh
# The screen should show the port statistics of port 0 and port 1

# Run a basic stateless UDP latency test using Trex's Python API.
terminal 3 > sudo docker exec -it pktgen bash
terminal 3 > cd /trex/v2.81/local
terminal 3 > python3 ./stl_test_burst.py --total_pkts 1000 --pps 100 --monitor_dur 10
```

If everything is configured correctly, you should see latency test results as follows:

```bash
--- Latency stats of the test stream:
Dropped: 0, Out-of-Order: 0
The average latency: 735.7880859375 usecs, total max: 86289 usecs, jitter: 1200 usecs
{1000: 17, 200: 3, 300: 7, 400: 12, 500: 9, 600: 5, 700: 2, 70000: 2, 800: 5, 80000: 1, 900: 7}
Latency test is passed!
```

3.  Tear down the setup with cleanups.

```bash
sudo ./benchmark-local.py --pktgen_image trex:v2.81 teardown
```

## Build from Source

Since FFPP utilizes latest fast packet IO technologies in both Linux kernel and user space, several dependencies must be installed in the development environment.

Sources running in user space (located in `./user`) are built with `Meson`.
Sources running in kernel space (located in `./kern`) are built with the makefile-based system provided by the [xdp-tools](https://github.com/xdp-project/xdp-tools) project.

-   Required dependencies:

  *   [Meson](https://mesonbuild.com/)(>=0.47.1)
  *   [Ninja](https://github.com/ninja-build/ninja)(>=1.10.0)
  *   [DPDK](https://core.dpdk.org/download/)(v20.08): DPDK's API and ABI could change in every release, so this library only supports a specific version.
  *   [xdp-tools](https://github.com/xdp-project/xdp-tools)(v0.0.3)
  *   [Jansson](https://digip.org/jansson/)
  *   [libzmq](https://zeromq.org/)
  *   python3-dev: Used for embedding Python.


If all dependencies are installed correctly, then you should be able to simply run following commands to build and install (**ONLY** user space library) both shared and
static library of FFPP:

```bash
make
make install
```


## FAQ

1.  Q: Why use C instead of higher-level or "modern" programming languages ?

A: This project is research oriented and the focus is on latency performance.
C is chosen since the libraries (DPDK and eBPF/XDP) used by this project are all written in C and provide C APIs
directly. It is easier to adapt and test latest features introduced in this libraries.
By the way, C is a simple and "old" language with a really small runtime, so there is not much magic there.
This makes the latency performance evaluation results more representative and "stable" for the algorithm used instead of the magic inside the runtime of
the programming language.
C has its limitations and security issues. However, they are not the focus of this project and it is just a PhD work :).


## Contact

*   Zuo Xiang (zuo.xiang@tu-dresden.de)
*   Malte Höweler (malte.hoeweler94@gmail.com)


## Licence

The user space code of this project is licensed under the MIT - see the [LICENSE](../../LICENSE) file for details.
The kernel space code of this project is licensed under GPL-2.0.
