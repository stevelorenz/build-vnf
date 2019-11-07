# Benchmark FFPP performance #

## Setup 1: Use ComNetsEmu emulator ##

1.  Install ComNetsEmu with [install_comnetsemu.sh](../../script/install_comnetsemu.sh).
1.  Build the container image for DPDK application with [run_dev_container.py](../util/run_dev_container.py) script.
    Go to the directory ../util/ and run `sudo python3 ./run_dev_container.py build_image`.
    Since DPDK is compiled from source code, this step may take some time.
1.  Build the container image with latency benchmark (lat_bm) tools with [./build.sh](./build.sh).
1.  Check if all requirements are ready:

    1. Check if ComNetsEmu python package is installed correctly with `sudo pip3 list | grep comnetsemu`
    1. Check if Docker-Py python package is installed correctly with `sudo pip3 list | grep docker`
    1. Check if required Docker images are built correctly with `sudo docker image ls` and check `ffpp` and `lat_bm` are
       in the list.

1.  Run UDP latency measurement on chain topology:

    1. For scenario without relay, so client and server are directly connected: `sudo python3 ./chain.py`.
    1. For scenario with relay running DPDK l2fwd between client and server: `sudo python3 ./chain.py add_relay`.
