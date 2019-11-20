# Benchmark FFPP performance #

## Setup 1: With ComNetsEmu Network Emulator ##

1.  Install ComNetsEmu with the script [install_comnetsemu.sh](../../script/install_comnetsemu.sh).
1.  Build the container image for DPDK application with the Python script [run_dev_container.py](../util/run_dev_container.py).
    Change the working directory to `../util/` and run `sudo python3 ./run_dev_container.py build_image`.
    Since DPDK is compiled from source code, this step may take some time.
1.  Build the container image with latency benchmark (*lat_bm*) tools with the script [./build.sh](./build.sh).
1.  Check if all requirements are ready:

    1. Check if ComNetsEmu python package is installed correctly with `sudo pip3 list | grep comnetsemu`
    1. Check if Docker-Py python package is installed correctly with `sudo pip3 list | grep docker`
    1. Check if required Docker images are built correctly with `sudo docker image ls` and check `ffpp` and `lat_bm` are
       in the list.

### Benchmark 1: Per-Packet Latency in the Simple Chain Topology ###

1.  Source code: [chain.py](./chain.py).
    Check the help and command line options with `sudo python3 ./chain.py -h`
    For example, run the DPDK l2fwd sample application on the relay. The client and server transmit UDP traffic:
    `sudo python3 ./chain.py`
