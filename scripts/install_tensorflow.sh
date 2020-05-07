#! /bin/bash
#
# install_tensorflow.sh
#
# About: Install Tensorflow on Ubuntu 16.04
#
PIP_VERSION="9.0.3"
echo "source the vnf_dev environment"

sudo apt-get install -y python3-pip
sudo -H pip3 install --upgrade pip=="$PIP_VERSION"
sudo -H pip3 install tensorflow
