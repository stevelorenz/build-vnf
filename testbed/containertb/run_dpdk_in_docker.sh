#! /bin/bash
#
# run_dpdk_in_container.sh
#
# About: Run DPDK inside a docker container
#

APP="dpdk_test"
IMAGE="dpdk_test:v0.1"

echo "*** Run DPDK app inside a docker container"

echo "# Start dpdk_test docker."
sudo docker container stop "$APP"
sudo docker container rm "$APP"
sudo docker run -dit --privileged \
    -v /sys/bus/pci/drivers:/sys/bus/pci/drivers -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev \
    -v /vagrant:/vagrant \
    --name "$APP" "$IMAGE" bash

echo "# Run DPDK example hello-world."
sudo docker exec "$APP" /root/dpdk/examples/helloworld/build/helloworld

echo "# Stop and remove $APP container."
sudo docker container stop "$APP"
sudo docker container rm "$APP"
