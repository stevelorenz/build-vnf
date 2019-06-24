#!/bin/bash
#
# About: Run the development container for yolo_pp VNF
#

if [[ $1 == "-b" ]]; then
    echo "Build yolo_pp image with ./Dockerfile.dpdk ..."
    sudo docker build -t yolo_pp --file ./Dockerfile .
elif [[ $1 == "-r" ]]; then
    echo "Run yolo_pp container"
    sudo docker run --rm --privileged \
        -v /sys/bus/pci/drivers:/sys/bus/pci/drivers -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev \
        -v $PWD:/yolo_pp -w /yolo_pp \
        -v /vagrant/dataset:/dataset \
        -v /vagrant/fastio_user:/fastio_user_dev \
        --name yolo_pp \
        -it yolo_pp bash

elif [[ $1 == "-t" ]]; then
    echo "ATtach to yolo_pp container"
    sudo docker exec -it yolo_pp bash

else
    echo "Unknown option! Options: -b, -r, -t"

fi
