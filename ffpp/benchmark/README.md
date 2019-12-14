# Benchmark FFPP performance #

Assume that you have logged into the vnf VM via SSH: `vagrant ssh vnf`

## Setup 1: With ComNetsEmu Network Emulator ##

1.  Install ComNetsEmu with the script [install_comnetsemu.sh](../../script/install_comnetsemu.sh): 
    `$ bash /vagrant/script/install_comnetsemu.sh`

1.  Build the container image for DPDK application with the Python script [run_dev_container.py](../util/run_dev_container.py):
    `$ cd /vagrant/ffpp/util/ && sudo python3 ./run_dev_container.py build_image`
    Since DPDK library is compiled from source code, this step should take some time.

1.  Build the container image with latency benchmark (*lat_bm*) tools with the script [./build.sh](./build.sh):
    `$ cd /vagrant/ffpp/benchmark && sudo bash ./build.sh`

1.  Check if all requirements are ready:

    1. Check if ComNetsEmu python package is installed correctly with `sudo pip3 list | grep comnetsemu`
    1. Check if Docker-Py python package is installed correctly with `sudo pip3 list | grep docker`
    1. Check if required Docker images are built correctly with `sudo docker image ls` and check `ffpp` and `lat_bm` are
       in the list.

Change the current directory to `/vagrant/ffpp/benchmark` before following steps.

### Benchmark 1: Per-Packet Latency in the Simple Chain Topology ###

Please run `bash ./setup.sh` for initial setups for DPDK, power monitors before running the application.
This setup is **not persistent** after rebooting, so run it again after each new rebooting.

-  Source code: [chain.py](./chain.py) for topology and performed tests.

-  Topology: Client --- Relay --- Server. Nodes are connected via OVS and redirect rules are added in OVS's flow table.

-  Run a simple per-packet latency benchmark with `sudo python3 ./chain.py`.
   Here the relay runs the DPDK l2fwd sample application to forward packets between client and server.
   The client uses [Sockperf](https://github.com/mellanox/sockperf) to measure the per-packet latency with different
   l4 protocols (UDP and TCP) and different speed (message per second). The under-load mode of Sockperf is used.

-  Check `sudo python3 ./chain.py -h` and source code for more details.
