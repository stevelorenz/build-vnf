#!/bin/bash
#
# About: Install Go (V1.12) on Ubuntu 18.04
# MAEK : - Default Go package (V1.10) on Ubuntu 18.04 does not support `go mod`
#

sudo apt-get update
sudo apt-get -y upgrade
# sudo apt-get remove -y golang-*

cd /opt || exit
sudo wget https://dl.google.com/go/go1.12.6.linux-amd64.tar.gz
sudo tar -xvf go1.12.6.linux-amd64.tar.gz
sudo rm go1.12.6.linux-amd64.tar.gz

echo "export GOROOT=/opt/go" >>${HOME}/.profile
echo "export PATH=$GOROOT/bin:$PATH" >>${HOME}/.profile

source ${HOME}/.profile
