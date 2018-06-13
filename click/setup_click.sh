#! /bin/bash
#
# About: Setup Click Modular Router
#        Both of user-level program and Linux kernel module are installed

#CLICK_VERSION="v2.0"
CLICK_VERSION="master"
LINUX_VERSION=$(uname -r)

sudo apt-get update
sudo apt-get install -y g++
sudo apt-get install -y linux-headers-"$LINUX_VERSION"

git clone https://github.com/kohler/click /home/vagrant/click

cd /home/vagrant/click || exit
git checkout -b $CLICK_VERSION
sudo chmod go+r /boot/System.map-"$LINUX_VERSION"
if [[ $1 = '-u' ]]; then
    # Only enable User-level
    ./configure --prefix=/usr/local
elif [[ $1 = '-uk' ]]; then
    # Enable Linux kernel module
    ./configure --prefix=/usr/local --enable-linuxmodule --with-linux=/usr/src/linux-headers-"$LINUX_VERSION" --with-linux-map=/boot/System.map-"$LINUX_VERSION"
fi
sudo make clean
sudo make install
