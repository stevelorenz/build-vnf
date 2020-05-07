#! /bin/bash
#
# About: Run the test docker container with shared volume
#

#CV_TAG="latest"
CV_TAG="contrib-opencv-3.4.2"
#CV_TAG="contrib-opencv-4.0.0"
if [[ $1 == "-cv" ]]; then
    echo "Run OpenCV container"
    sudo docker -D run --rm --name rpa -v /vagrant/dataset:/dataset -v/vagrant/model:/model -v /vagrant/app:/app -w /app/distributed_rcnn -it jjanzic/docker-python3-opencv:"$CV_TAG" bash
elif [[ $1 == "-tf" ]]; then
    echo "Run Tensorflow container"
    sudo docker run --rm -v /vagrant/dataset:/dataset -v/vagrant/model:/model -v /vagrant/app:/app -w /app/distributed_rcnn -it tensorflow/tensorflow bash
elif [[ $1 == "-pt" ]]; then
    echo "Run PyTorch container"
    sudo docker run --rm -v /vagrant/dataset:/dataset -v/vagrant/model:/model -v /vagrant/app:/app -it pytorch/pytorch bash
fi
