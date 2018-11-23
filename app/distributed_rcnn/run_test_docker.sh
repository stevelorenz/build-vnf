#! /bin/bash
#
# About: Run the test docker container with shared volume
#

#CV_TAG="latest"
CV_TAG="contrib-opencv-3.4.2"
#CV_TAG="contrib-opencv-4.0.0"
if [[ $1 == "-cv" ]]; then
    echo "Run OpenCV container"
    sudo docker run -v /vagrant/dataset:/dataset -v/vagrant/model:/model -v /vagrant/app:/app -it jjanzic/docker-python3-opencv:"$CV_TAG" bash
elif [[ $1 == "-tf" ]]; then
    echo "Run Tensorflow container"
    sudo docker run -v /vagrant/dataset:/dataset -v/vagrant/model:/model -v /vagrant/app:/app -it tensorflow/tensorflow bash
elif [[ $1 == "-pt" ]]; then
    sudo docker run -v /vagrant/dataset:/dataset -v/vagrant/model:/model -v /vagrant/app:/app -it pytorch/pytorch bash
fi
