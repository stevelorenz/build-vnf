#! /bin/bash
#
# install_mininet
#
# About: Install mininet on Ubuntu 16.04
#

cd /home/vagrant
mkdir mininet
cd mininet
git clone git://github.com/mininet/mininet
cd mininet
git checkout -b 2.2.2 2.2.2
cd ..

alias python=python3 >> ~/.bashrc
source ~/.bashrc

echo "install mininet"
sudo bash ./mininet/util/install.sh -a
echo "install other dependences"

sudo mn --test pingall
