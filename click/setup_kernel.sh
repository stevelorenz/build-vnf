#!/bin/bash
# About: Install Kernel v3.5 for building Click kernel module
# Email: xianglinks@gmail.com

sudo apt install -y module-init-tools

cd /tmp
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v3.5-quantal/linux-headers-3.5.0-030500_3.5.0-030500.201207211835_all.deb
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v3.5-quantal/linux-headers-3.5.0-030500-generic_3.5.0-030500.201207211835_amd64.deb
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v3.5-quantal/linux-image-3.5.0-030500-generic_3.5.0-030500.201207211835_amd64.deb
sudo dpkg -i *.deb
