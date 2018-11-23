#! /bin/bash
#
# About: Pull and build some required docker images
#

TEST_CV_TAG="contrib-opencv-4.0.0"  # WARN: latest version, might be unstable
echo "# Build OpenCV docker images"
sudo docker pull "jjanzic/docker-python3-opencv"
sudo docker pull "jjanzic/docker-python3-opencv:contrib-opencv-3.4.2"
sudo docker pull "jjanzic/docker-python3-opencv:$TEST_CV_TAG"

echo "# Build Tensorflow docker image"
sudo docker pull tensorflow/tensorflow

echo "# Build Pytorch docker image"
sudo docker pull pytorch/pytorch
