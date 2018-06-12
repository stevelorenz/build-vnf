#! /bin/bash
#
# About: Setup Click Modular Router
#        Both of user-level program and Linux kernel module are installed

CLICK_VERSION="v2.0"
LINUX_VERSION=$(uname -r)

sudo apt-get install -y linux-headers-$LINUX_VERSION

git clone https://github.com/kohler/click /home/vagrant/click

cd /home/vagrant/click || exit
git checkout -b $CLICK_VERSION
sudo chmod go+r /boot/System.map-$LINUX_VERSION
# Only enable User-level
./configure --prefix=/usr/local
# Enable Linux kernel module, can not be compiled currently on 4.4.0, Ubuntu 16.04.
#./configure --prefix=/usr/local --enable-linuxmodule --with-linux=/usr/src/linux-headers-$LINUX_VERSION --with-linux-map=/boot/System.map-$LINUX_VERSION
sudo make clean
sudo make install
