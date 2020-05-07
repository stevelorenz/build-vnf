#! /bin/bash
#
# install_docker.sh
#
# About: Install Docker CE on Ubuntu
#

# Docker CE is supported on Ubuntu on x86_64, armhf, s390x

sudo apt remove docker docker-engine docker.io
sudo apt update

sudo apt-get install -y \
    apt-transport-https \
    ca-certificates \
    curl \
    software-properties-common

curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -

sudo apt-key fingerprint 0EBFCD88

sudo add-apt-repository \
    "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
    $(lsb_release -cs) \
    stable"

sudo apt update
sudo apt install -y docker-ce

echo "Verify that Docker CE is installed correctly."
sudo docker run hello-world
