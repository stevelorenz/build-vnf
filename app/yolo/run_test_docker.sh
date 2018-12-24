#! /bin/bash
#
# About: Run the test docker container with shared volume
#

#CV_TAG="latest"
CV_TAG="contrib-opencv-3.4.2"
#CV_TAG="contrib-opencv-4.0.0"
if [[ $1 == "-cv" ]]; then
    echo "Run OpenCV container"
    sudo docker run --rm -v /vagrant/dataset:/dataset -v/vagrant/model:/model -v /vagrant/app:/app -w /app/distributed_rcnn -it jjanzic/docker-python3-opencv:"$CV_TAG" bash
elif [[ $1 == "-dn" ]]; then
    echo "Run darknet container"
    sudo docker run --rm --privileged \
        -v /sys/bus/pci/drivers:/sys/bus/pci/drivers -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev \
        -v /vagrant/dataset:/dataset -v/vagrant/model:/model -v /vagrant/app:/app -w /app/yolo -it darknet bash
fi
