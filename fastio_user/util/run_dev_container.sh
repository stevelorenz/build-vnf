#!/usr/bin/env bash
#
# About: Run the development container for fastio_user lib
#

set -e
set -o nounset

declare -r opt="${1-"default"}"

cd ../ || exit

if [[ ! -e ./Dockerfile.dpdk ]]; then
    echo "Run this script in util directory."
    echo "The ../ directory should contain ./Dockerfile.dpdk"
    exit 1
fi

if [[ $opt == "-b" ]]; then
    echo "Build dpdk image with ./Dockerfile.dpdk ..."
    sudo docker build -t fastio_user --file ./Dockerfile.dpdk .

elif [[ $opt == "-m" ]]; then
    echo "Build dpdk image with ./Dockerfile.dpdk ..."
    sudo docker run --rm --privileged \
        -v /sys/bus/pci/drivers:/sys/bus/pci/drivers -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev \
        -v "$PWD":/fastio_user -w /fastio_user \
        -it fastio_user make

elif [[ $opt == "-k" ]]; then
    echo "Run container for performance benchmarking"
    sudo docker run --rm --privileged \
        -v /usr/local/var/run/openvswitch:/var/run/openvswitch \
        -v /sys/bus/pci/drivers:/sys/bus/pci/drivers -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev \
        -v "$PWD":/fastio_user -w /fastio_user \
        -v /vagrant/dataset:/dataset \
        -it fastio_user bash

elif [[ $opt == "-h" || $opt == "--help" ]]; then
    echo "Usage: ./run_dev_container.sh [option]"
    echo "Option:"
    echo "  -b: Build docker image with ./Dockerfile.dpdk"
    echo "  -m: Run dev container and build the shared lib"
    echo "  -k: Run dev container for benchmarking and enter shell"
    echo "      - OVS-DPDK's path is also mounted"
    echo ""
    echo "  empty: Run dev container and enter shell"

elif [[ $opt == "default" ]]; then
    echo "Run container for development and tests"
    sudo docker run --rm --privileged \
        -v /sys/bus/pci/drivers:/sys/bus/pci/drivers -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev \
        -v "$PWD":/fastio_user -w /fastio_user \
        -v /vagrant/dataset:/dataset \
        -it fastio_user bash

fi
