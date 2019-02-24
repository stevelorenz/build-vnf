#! /bin/bash
#
# About: Test running MoonGen inside a docker container
#

if [[ $1 == "-b" ]]; then
    echo "Build MoonGen image with ../../dockerfiles/Dockerfile.moongen ..."
    sudo docker build -t moongen --file ../../dockerfiles/Dockerfile.moongen .
else
    echo "Run MoonGen container"
    sudo docker run --rm --privileged \
        -v /sys/bus/pci/drivers:/sys/bus/pci/drivers -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev \
        -v /vagrant/perf/moongen/:/test \
        -w /root/MoonGen -it moongen bash

fi
