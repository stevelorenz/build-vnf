#!/bin/bash
#
# About: Run the development container for fastio_user lib
#

if [[ $1 == "-b" ]]; then
    echo "Build dpdk image with ./Dockerfile.dpdk ..."
    sudo docker build -t fastio_user --file ./Dockerfile.dpdk .

elif [[ $1 == "-m" ]]; then
    echo "Build dpdk image with ./Dockerfile.dpdk ..."
    sudo docker run --rm --privileged \
        -v /sys/bus/pci/drivers:/sys/bus/pci/drivers -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev \
        -v $PWD:/fastio_user -w /fastio_user \
        -it fastio_user make

elif [[ $1 == "-k" ]]; then
    echo "Run container for performance benchmarking"
    sudo docker run --rm --privileged \
        -v /usr/local/var/run/openvswitch:/var/run/openvswitch \
        -v /sys/bus/pci/drivers:/sys/bus/pci/drivers -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev \
        -v $PWD:/fastio_user -w /fastio_user \
        -v /vagrant/dataset:/dataset \
        -it fastio_user bash

else
    echo "Run container for development and tests"
    sudo docker run --rm --privileged \
        -v /sys/bus/pci/drivers:/sys/bus/pci/drivers -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev \
        -v $PWD:/fastio_user -w /fastio_user \
        -v /vagrant/dataset:/dataset \
        -it fastio_user bash

fi
